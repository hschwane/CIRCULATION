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
    // create initial conditions
    for(int i : mpu::Range<int>(m_grid->size()))
    {
        float velX = 0.0f;
        float velY = 0.0f;
        float geopotential = 1.0f;

        m_grid->initialize<AT::geopotential>(i, geopotential);
        m_grid->initialize<AT::velocityX>(i, velX);
        m_grid->initialize<AT::velocityY>(i, velY);
    }

    // initialize some cells to a higher potential
    float highPotential = 10.0f;
    int3 center = int3{ m_cs->getNumGridCells3d().x /2, m_cs->getNumGridCells3d().y /2, 0};
    int centerId = m_cs->getCellId(center);

    m_grid->initialize<AT::geopotential>( centerId, highPotential);
    m_grid->initialize<AT::geopotential>( m_cs->getLeftNeighbor(centerId), highPotential);
    m_grid->initialize<AT::geopotential>( m_cs->getRightNeighbor(centerId), highPotential);
    m_grid->initialize<AT::geopotential>( m_cs->getBackwardNeighbor(centerId), highPotential);
    m_grid->initialize<AT::geopotential>( m_cs->getForwardNeighbor(centerId), highPotential);

    // swap buffers and ready for rendering
    m_grid->swapAndRender();

    // reset simulation state
    m_totalSimulatedTime = 0.0f;
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
                                        mpu::VectorReference<float> phiPlusK, float timestep)
{
    csT cs = coordinateSystem;

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

            // compute geopotential time derivative dPhi/dt
            const float divv = divergence2d( velLeftX, velRightX, velBackY, velForY, cellPos, cs);
            const float dphi_dt = -divv * phi;

            // compute values at t+1
            const float nextPhi = phi + dphi_dt * timestep;
            grid.write<AT::geopotential>(cellId,nextPhi);
        }
}

template <typename csT>
__global__ void shallowWaterSimulationB(ShallowWaterGrid::ReferenceType grid, csT coordinateSystem,
                                        mpu::VectorReference<const float> phiPlusK, float timestep)
{
    csT cs = coordinateSystem;

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
            const float nextVelX = velX + dvX_dt * timestep;
            const float nextVelY = velY + dvY_dt * timestep;
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

    shallowWaterSimulationA<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs,m_phiPlusKBuffer.getVectorReference(),m_timestep);
    shallowWaterSimulationB<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs,m_phiPlusKBuffer.getVectorReference(),m_timestep);

    m_totalSimulatedTime += m_timestep;
}

GridBase& ShallowWaterModel::getGrid()
{
    return *m_grid;
}

std::string ShallowWaterModel::getDisplayName()
{
    return "Shallow Water Model";
}