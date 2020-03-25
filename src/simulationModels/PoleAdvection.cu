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
#include "PoleAdvection.h"

#include <mpUtils/mpUtils.h>
#include <mpUtils/mpGraphics.h>
#include <mpUtils/mpCuda.h>

#include "../GridReference.h"
#include "../finiteDifferences.h"
#include "../boundaryConditions.h"
//--------------------

// function definitions of the ShallowWaterModel class
//-------------------------------------------------------------------

void PoleAdvection::showCreationOptions()
{
    ImGui::Text("Test Case number 1 from David L. Williamson 1992.");
    ImGui::DragFloat("Wind Angle offset (alpha)", &m_alpha, 0.001f,0.0,M_PI_2);
    ImGui::DragFloat("Wind Velocity (u0)", &m_u0, 0.001f);
    ImGui::DragFloat("Earth radius (a)", &m_earthRadius);
    ImGui::DragFloat("Angular Velocity",&m_angularVelocity,0.00001f,0.00001,5.0f,"%.8f");
}

void PoleAdvection::showBoundaryOptions(const CoordinateSystem& cs)
{
}

void PoleAdvection::showSimulationOptions()
{
    ImGui::DragFloat("Angular Velocity",&m_angularVelocity,0.00001f,0.00001,5.0f,"%.5f");

    ImGui::DragFloat("Geopotential diffusion",&m_geopotDiffusion,0.00001f,0.00001,1.0,"%.5f");
    ImGui::Checkbox("Use Leapfrog",&m_useLeapfrog);
    ImGui::DragFloat("Timestep",&m_timestep,0.000001,0.000001f,1.0,"%.6f");
    ImGui::Text("Simulated Time units: %f", m_totalSimulatedTime);
}

std::shared_ptr<GridBase> PoleAdvection::recreate(std::shared_ptr<CoordinateSystem> cs)
{
    m_cs = cs;
    m_grid = std::make_shared<ShallowWaterGrid>(m_cs->getNumGridCells());
    m_phiPlusKBuffer.resize(m_cs->getNumGridCells());
    m_vortPlusCor.resize(m_cs->getNumGridCells());

    if(m_cs->getType() != CSType::geographical2d)
    {
        logERROR("PoleAdvection") << "Pole advection test loaded in cartesian cordinates!";
        tinyfd_messageBox("Error","Advection Test only works for geographical coordinates",
                          "ok", "error",1);
        return m_grid;
    }

    reset();
    return m_grid;
}

void PoleAdvection::reset()
{
    m_grid->cacheOverwrite();

    float cosAlpha = cos(m_alpha);
    float sinAlpha = sin(m_alpha);

    // create initial conditions using gaussian
    for(int i : mpu::Range<int>(m_grid->size()))
    {
        float3 c = m_cs->getCellCoordinate(i);

        float velX = m_u0*(cos(c.y)*cosAlpha + sin(c.y)*cos(c.x)*sinAlpha);
        float velY = -m_u0*sin(c.y)*sinAlpha;

        float geopotential = 0;

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

std::unique_ptr<Simulation> PoleAdvection::clone() const
{
    return std::make_unique<PoleAdvection>(*this);
}

void PoleAdvection::simulateOnce()
{
    if(m_cs->getType() != CSType::geographical2d)
        return;

    simulateOnceImpl(static_cast<GeographicalCoordinates2D&>( *(this->m_cs)));
}

__global__ void poleAdvectionA(ShallowWaterGrid::ReferenceType grid, GeographicalCoordinates2D coordinateSystem,
                               mpu::VectorReference<float> phiPlusK, mpu::VectorReference<float> vortPlusCor,
                               float timestep, bool useLeapfrog, float diffusion, float corOrAngvel)
{
    GeographicalCoordinates2D cs = coordinateSystem;

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
            const float velForX  = grid.read<AT::velocityX>(cs.getForwardNeighbor(cellId)); // used for vorticity
            const float velRightY  = grid.read<AT::velocityY>(cs.getRightNeighbor(cellId)); // used for vorticity

            // compute kinetic energy per unit mass
            const float velX = (velLeftX + velRightX) * 0.5f;
            const float velY = (velForY + velBackY) * 0.5f;
            const float kinEnergy = (velX * velX + velY * velY) * 0.5f;
            phiPlusK[cellId] = kinEnergy + phi;

            // calculate vorticity and coriolis parameter
            // if this looks strange consider where values are located on the C grid
            const float2 vortPos = cellPos + 0.5f * make_float2(cs.getCellSize()); // position where vorticity is computed
            const float vort = curl2d(velForY, velRightY, velRightX, velForX, vortPos, cs);
            float cor;
            if(cs.getType() == CSType::geographical2d)
                cor = 2*corOrAngvel*sin(vortPos.y);
            else if(cs.getType() == CSType::cartesian2d)
                cor = corOrAngvel;
            else
                cor = 0.0f;
            vortPlusCor[cellId] = vort + cor;

            // write potential vorticity
            grid.write<AT::potentialVort>(cellId, abs(vort+cor) / phi);

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

__global__ void poleAdvectionB(ShallowWaterGrid::ReferenceType grid, GeographicalCoordinates2D coordinateSystem,
                               mpu::VectorReference<const float> phiPlusK, mpu::VectorReference<float> vortPlusCor,
                               float timestep, bool useLeapfrog)
{
    GeographicalCoordinates2D cs = coordinateSystem;

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
            const float vortCorLeft = vortPlusCor[cs.getLeftNeighbor(cellId)];
            const float vortCorBack = vortPlusCor[cs.getBackwardNeighbor(cellId)];
            const float vortCor = vortPlusCor[cellId];
            const float velX = grid.read<AT::velocityX>(cellId);
            const float velY = grid.read<AT::velocityY>(cellId);

            // compute dvX/dt and dvY/dt
            const float2 gradPhiK = gradient2d(phiK,phiKRight,phiK,phiKForward,cellPos,cs);
            const float dvX_dt = (vortCor+vortCorBack)*0.5f*velY -gradPhiK.x;
            const float dvY_dt = -(vortCor+vortCorLeft)*0.5f*velX -gradPhiK.y;

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


void PoleAdvection::simulateOnceImpl(GeographicalCoordinates2D& cs)
{
    dim3 blocksize{16,16,1};
    dim3 numBlocks{ static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().x ,blocksize.x)),
                    static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().y ,blocksize.y)), 1};

    poleAdvectionA << < numBlocks, blocksize >> > (m_grid->getGridReference(),cs,m_phiPlusKBuffer.getVectorReference(),
            m_vortPlusCor.getVectorReference(), m_timestep, !m_firstTimestep && m_useLeapfrog,
            m_geopotDiffusion, m_angularVelocity);
    poleAdvectionB << < numBlocks, blocksize >> > (m_grid->getGridReference(),cs,m_phiPlusKBuffer.getVectorReference(),
            m_vortPlusCor.getVectorReference(), m_timestep, !m_firstTimestep && m_useLeapfrog);

    m_totalSimulatedTime += m_timestep;
    m_firstTimestep = false;
}

GridBase& PoleAdvection::getGrid()
{
    return *m_grid;
}

std::string PoleAdvection::getDisplayName()
{
    return "Shallow Water Model";
}