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
#include <mpUtils/external/imgui/imgui_internal.h>
#include "ShallowWaterModel.h"
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
    ImGui::DragFloat("Timestep",&m_timestep,0.0001,0.0001f,1.0,"%.4f");
    ImGui::Text("Simulated Time units: %f", m_totalSimulatedTime);
}

std::shared_ptr<GridBase> ShallowWaterModel::recreate(std::shared_ptr<CoordinateSystem> cs)
{
    m_cs = cs;
    m_grid = std::make_shared<ShallowWaterGrid>(m_cs->getNumGridCells());

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
        float geopotential = 10.0f;

        m_grid->initialize<AT::geopotential>(i, geopotential);
        m_grid->initialize<AT::velocityX>(i, velX);
        m_grid->initialize<AT::velocityY>(i, velY);
    }

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
__global__ void shallowWaterSimulation(ShallowWaterGrid::ReferenceType grid, csT coordinateSystem, float timestep)
{
    csT cs = coordinateSystem;

    for(int x : mpu::gridStrideRange( cs.hasBoundary().x, cs.getNumGridCells3d().x-cs.hasBoundary().x ))
        for(int y : mpu::gridStrideRangeY( cs.hasBoundary().y, cs.getNumGridCells3d().y-cs.hasBoundary().y ))
        {
            int3 cell{x,y,0};
            int cellId = cs.getCellId(cell);
            float2 cellPos = make_float2( cs.getCellCoordinate3d(cell) );

            // read values of quantities
            const float Phi = grid.read<AT::geopotential>(cellId);
            const float velRightX = grid.read<AT::velocityX>(cellId);
            const float velForY   = grid.read<AT::velocityY>(cellId);

            // compute geopotential time derivative dPhi/dt
            const float velLeftX  = grid.read<AT::velocityX>(cs.getLeftNeighbor(cellId));
            const float velBackY  = grid.read<AT::velocityY>(cs.getBackwardNeighbor(cellId));

            const float divv = divergence2d( velLeftX, velRightX, velBackY, velForY, cellPos, cs);
            const float dPhi_dt = -divv * Phi;

            // compute wind velocity time derivative

            // compute values at t+1
            const float nextPhi = Phi + dPhi_dt * timestep;
            grid.write<AT::geopotential>(cellId,nextPhi);
        }
}


template <typename csT>
void ShallowWaterModel::simulateOnceImpl(csT& cs)
{
    dim3 blocksize{16,16,1};
    dim3 numBlocks{ static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().x ,blocksize.x)),
                    static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().y ,blocksize.y)), 1};

    shallowWaterSimulation<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs,m_timestep);
}

GridBase& ShallowWaterModel::getGrid()
{
    return *m_grid;
}

std::string ShallowWaterModel::getDisplayName()
{
    return "Shallow Water Model";
}