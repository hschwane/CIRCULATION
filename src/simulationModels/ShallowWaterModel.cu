/*
 * CIRCULATION
 * ShallowWaterModel.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the ShallowWaterModel class
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "ShallowWaterModel.h"

#include <mpUtils/mpUtils.h>
#include <mpUtils/mpGraphics.h>
#include <mpUtils/mpCuda.h>

#include "../GridReference.h"
#include "../coordinateSystems/CartesianCoordinates2D.h"
#include "../coordinateSystems/GeographicalCoordinates2D.h"
#include "../finiteDifferences.h"
#include "../boundaryConditions.h"
//--------------------

// function definitions of the ShallowWaterModel class
//-------------------------------------------------------------------

void ShallowWaterModel::showCreationOptions()
{

}

void ShallowWaterModel::showBoundaryOptions(const CoordinateSystem& cs)
{
}

void ShallowWaterModel::showSimulationOptions()
{
    ImGui::DragFloat("Geopotential diffusion",&m_geopotDiffusion,0.00001f,0.00001,1.0,"%.5f");
    ImGui::Checkbox("Use Leapfrog",&m_useLeapfrog);
    ImGui::DragFloat("Timestep",&m_timestep,0.000001,0.000001f,1.0,"%.6f");
    ImGui::Text("Simulated Time units: %f", m_totalSimulatedTime);
}

std::shared_ptr<GridBase> ShallowWaterModel::recreate(std::shared_ptr<CoordinateSystem> cs)
{
    m_cs = cs;
    m_grid = std::make_shared<ShallowWaterGrid>(m_cs->getNumGridCells());
    m_phiPlusKBuffer.resize(m_cs->getNumGridCells());

    // select coordinate system
    switch(m_cs->getType())
    {
        case CSType::cartesian2d:
            m_simOnceFunc = [this](){ this->simulateOnceImpl( static_cast<CartesianCoordinates2D&>( *(this->m_cs)) ); };
            break;
        case CSType::geographical2d:
            m_simOnceFunc = [this](){ this->simulateOnceImpl( static_cast<GeographicalCoordinates2D&>( *(this->m_cs)) ); };
            break;
    }

    reset();
    return m_grid;
}

void ShallowWaterModel::reset()
{

    m_grid->cacheOverwrite();

    float3 center = m_cs->getMinCoord() + (m_cs->getMaxCoord() - m_cs->getMinCoord())*0.5f;
    logINFO("bla") << m_cs->getMinCoord() << m_cs->getMaxCoord() << center;
    // create initial conditions using gaussian
    for(int i : mpu::Range<int>(m_grid->size()))
    {
        float velX = 0.0f;
        float velY = 0.0f;

        float3 c = m_cs->getCellCoordinate(i);

        float geopotential = fmax(1, 0.1 * glm::gauss<float>(c.x,center.x, 0.1f) * glm::gauss<float>(c.y,center.y, 0.1f));


        m_grid->initialize<AT::geopotential>(i, geopotential);
        m_grid->initialize<AT::velocityX>(i, velX);
        m_grid->initialize<AT::velocityY>(i, velY);
    }
    m_grid->pushCachToDevice();

    // swap buffers and ready for rendering
    m_grid->swapAndRender();

    // reset simulation state
    m_totalSimulatedTime = 0.0f;
    m_firstTimestep = true;
}

std::unique_ptr<Simulation> ShallowWaterModel::clone() const
{
    return std::make_unique<ShallowWaterModel>(*this);
}

void ShallowWaterModel::simulateOnce()
{
    m_simOnceFunc(); // calls correct template specialization
}

template <typename csT>
__global__ void shallowWaterSimulationA(ShallowWaterGrid::ReferenceType grid, csT coordinateSystem,
                                        mpu::VectorReference<float> phiPlusK, float timestep, bool useLeapfrog, float diffusion)
{
    csT cs = coordinateSystem;

    // updates geopotential for all non boundary cells
    // also calculates kinetic energy per unit mass
    for(int x : mpu::gridStrideRange( cs.hasBoundary().x, cs.getNumGridCells3d().x-cs.hasBoundary().x ))
        for(int y : mpu::gridStrideRangeY( cs.hasBoundary().y, cs.getNumGridCells3d().y-cs.hasBoundary().y ))
        {
            int3 cell{x,y,0};
            int cellId = cs.getCellId(cell);
            float2 cellPos = make_float2( cs.getCellCoordinate3d(cell) );

            // read values of quantities
            const float phi = grid.read<AT::geopotential>(cellId);
            const float velRightX = grid.read<AT::velocityX>(cellId);
            const float velForY   = grid.read<AT::velocityY>(cellId);
            const float velLeftX  = grid.read<AT::velocityX>(cs.getLeftNeighbor(cellId));
            const float velBackY  = grid.read<AT::velocityY>(cs.getBackwardNeighbor(cellId));

            // compute kinetic energy per unit mass
            const float velX = (velLeftX + velRightX) * 0.5f;
            const float velY = (velForY + velBackY) * 0.5f;
            const float kinEnergy = (velX * velX + velY * velY) * 0.5f;
            phiPlusK[cellId] = kinEnergy + phi;

            // compute geopotential advection time derivative dPhi/dt
            const float divv = divergence2d( velLeftX, velRightX, velBackY, velForY, cellPos, cs);
            float dphi_dt = -divv * phi;

            if(diffusion > 0)
            {
                const float phiLeft = grid.read<AT::geopotential>(cs.getLeftNeighbor(cellId));
                const float phiRight = grid.read<AT::geopotential>(cs.getRightNeighbor(cellId));
                const float phiFor = grid.read<AT::geopotential>(cs.getForwardNeighbor(cellId));
                const float phiBack = grid.read<AT::geopotential>(cs.getBackwardNeighbor(cellId));

                // compute geopotential diffusion
                const float lapphi = laplace2d(phiLeft,phiRight,phiBack,phiFor,phi,cellPos,cs);
                dphi_dt += diffusion * lapphi;
            }

            // compute values at t+1
            float nextPhi;
            if(useLeapfrog)
            {
                const float prevPhi = grid.readPrev<AT::geopotential>(cellId);
                nextPhi = prevPhi + dphi_dt * 2.0f*timestep;
            }
            else
                nextPhi = phi + dphi_dt * timestep;

            grid.write<AT::geopotential>(cellId,nextPhi);
        }
}

template <typename csT>
__global__ void shallowWaterSimulationB(ShallowWaterGrid::ReferenceType grid, csT coordinateSystem,
                                        mpu::VectorReference<const float> phiPlusK, float timestep, bool useLeapfrog)
{
    csT cs = coordinateSystem;

    // updates all non boundary velocities
    // TODO: handle velocities parallel to the boundary
    for(int x : mpu::gridStrideRange( cs.hasBoundary().x, cs.getNumGridCells3d().x-2*cs.hasBoundary().x ))
        for(int y : mpu::gridStrideRangeY( cs.hasBoundary().y, cs.getNumGridCells3d().y-2*cs.hasBoundary().y ))
        {
            int3 cell{x,y,0};
            int cellId = cs.getCellId(cell);
            float2 cellPos = make_float2( cs.getCellCoordinate3d(cell) );

            // read values of quantities
            const float phiKRight = phiPlusK[cs.getRightNeighbor(cellId)];
            const float phiKForward = phiPlusK[cs.getForwardNeighbor(cellId)];
            const float phiK = phiPlusK[cellId];
            const float velX = grid.read<AT::velocityX>(cellId);
            const float velY = grid.read<AT::velocityY>(cellId);

            // compute dvX/dt and dvY/dt
            const float2 gradPhiK = gradient2d(phiK,phiKRight,phiK,phiKForward,cellPos,cs);
            const float dvX_dt = -gradPhiK.x;
            const float dvY_dt = -gradPhiK.y;

            // compute values at t+1
            float nextVelX;
            float nextVelY;
            if(useLeapfrog)
            {
                const float prevVelX = grid.readPrev<AT::velocityX>(cellId);
                const float prevVelY = grid.readPrev<AT::velocityY>(cellId);

                nextVelX = prevVelX + dvX_dt * 2.0f*timestep;
                nextVelY = prevVelY + dvY_dt * 2.0f*timestep;
            }
            else
            {
                nextVelX = velX + dvX_dt * timestep;
                nextVelY = velY + dvY_dt * timestep;
            }
            grid.write<AT::velocityX>(cellId,nextVelX);
            grid.write<AT::velocityY>(cellId,nextVelY);
        }
}


template <typename csT>
void ShallowWaterModel::simulateOnceImpl(csT& cs)
{
    dim3 blocksize{16,16,1};
    dim3 numBlocks{ static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().x ,blocksize.x)),
                    static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().y ,blocksize.y)), 1};

    shallowWaterSimulationA<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs,m_phiPlusKBuffer.getVectorReference(),
            m_timestep, !m_firstTimestep && m_useLeapfrog, m_geopotDiffusion);
    shallowWaterSimulationB<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs,m_phiPlusKBuffer.getVectorReference(),
            m_timestep, !m_firstTimestep && m_useLeapfrog);

    m_totalSimulatedTime += m_timestep;
    m_firstTimestep = false;
}

GridBase& ShallowWaterModel::getGrid()
{
    return *m_grid;
}

std::string ShallowWaterModel::getDisplayName()
{
    return "Shallow Water Model";
}