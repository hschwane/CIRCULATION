/*
 * CIRCULATION
 * TestSimulation.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the TestSimulation class
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "TestSimulation.h"
#include "../GridReference.h"
#include "../coordinateSystems/CartesianCoordinates2D.h"
#include "../coordinateSystems/GeographicalCoordinates2D.h"
//--------------------

// function definitions of the TestSimulation class
//-------------------------------------------------------------------

void TestSimulation::drawCreationOptions()
{
    ImGui::Checkbox("Random Vectors", &m_randomVectors);
    if(!m_randomVectors)
        ImGui::DragFloat2("Vector", &m_vectorValue.x);
}

std::shared_ptr<GridBase> TestSimulation::recreate(std::shared_ptr<CoordinateSystem> cs)
{
    m_cs = cs;
    m_grid = std::make_shared<TestSimGrid>(m_cs->getNumGridCells());

    // generate some data
    std::default_random_engine rng(mpu::getRanndomSeed());
    std::normal_distribution<float> dist(10,4);
    std::normal_distribution<float> vdist(0,4);

    for(int i : mpu::Range<int>(m_grid->size()))
    {
        float density = fmax(0,dist(rng));
        float velX = vdist(rng);
        float velY = vdist(rng);

        m_grid->write<AT::density>(i,density);
        if(m_randomVectors)
        {
            m_grid->write<AT::velocityX>(i, velX);
            m_grid->write<AT::velocityY>(i, velY);
        }
        else {
            m_grid->write<AT::velocityX>(i, m_vectorValue.x);
            m_grid->write<AT::velocityY>(i, m_vectorValue.y);
        }
    }

    // swap buffers and ready for rendering
    m_grid->swapAndRender();

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

    return m_grid;
}

std::unique_ptr<Simulation> TestSimulation::clone() const
{
    return std::make_unique<TestSimulation>(*this);
}

void TestSimulation::showGui(bool* show)
{
    ImGui::SetNextWindowSize({0,0},ImGuiCond_FirstUseEver);
    if(ImGui::Begin("RenderDemoSimulation",show))
    {
        std::string state;
        if(m_isPaused)
            ImGui::Text("State: Paused");
        else
            ImGui::Text("State: running");

        ImGui::Text("This is a rendering demo, so the simulation does nothing. There are also no settings.");
    }
    ImGui::End();
}

void TestSimulation::simulateOnce()
{
    m_simOnceFunc(); // calls correct template specialization
}

//CUDAHOSTDEV float centralDeriv(float left, float right, float delta)
//{
//    return (right-left) / 2.0f*delta;
//}

//CUDAHOSTDEV float2 gradient2D()

template <typename csT>
__global__ void testSimulation(TestSimGrid::ReferenceType grid, csT cs)
{
    for(int x : mpu::gridStrideRange( 1, cs.getNumGridCells3d().x-1 ))
        for(int y : mpu::gridStrideRangeY( 1, cs.getNumGridCells3d().y-1 ))
    {
        int3 cell{x,y,0};
        int cellId = cs.getCellId(cell);

        float rho = grid.read<AT::density>(cellId);
        float velx = grid.read<AT::velocityX>(cellId);
        float vely = grid.read<AT::velocityY>(cellId);

        grid.write<AT::velocityX>(cellId, velx);
        grid.write<AT::velocityY>(cellId, vely);
        grid.write<AT::density>(cellId,rho);

        // calculate gradient using central difference
        // since we use the density at at i and i+1 we get the gradient halfway in between the cells,
        // on the edge between cell i and i+1
        float rhoRight     = grid.read<AT::density>(cs.getRightNeighbor(cellId));
        float rhoForward   = grid.read<AT::density>(cs.getForwardNeighbor(cellId));

        float2 gradRho;
        gradRho.x = (rhoRight - rho) / cs.getCellSize().x;
        gradRho.y = (rhoForward - rho) / cs.getCellSize().y;

        grid.write<AT::densityGradX>(cellId, gradRho.x);
        grid.write<AT::densityGradY>(cellId, gradRho.y);

        // calculate divergence of the velocity field
        // remember, velocities are defined half way between the nodes,
        // we want the divergence at the node, so we get a central difference by looking at the velocities left and backwards from us
        // and compare them to our velocities
        float velLeft = grid.read<AT::velocityX>(cs.getLeftNeighbor(cellId));
        float velBackward = grid.read<AT::velocityX>(cs.getBackwardNeighbor(cellId));

        float velDiv =  ( (velx-velLeft) / cs.getCellSize().x )
                      + ( (vely-velBackward) / cs.getCellSize().x );

        grid.write<AT::velocityDiv>(cellId, velDiv);
    }
}

template <typename csT>
void TestSimulation::simulateOnceImpl(csT& cs)
{
    dim3 blocksize{16,16,1};
    dim3 numBlocks{ static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().x ,blocksize.x)),
                    static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().y ,blocksize.y)), 1};

    testSimulation<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs);
}

template void TestSimulation::simulateOnceImpl<CartesianCoordinates2D>(CartesianCoordinates2D& cs);

GridBase& TestSimulation::getGrid()
{
    return *m_grid;
}
