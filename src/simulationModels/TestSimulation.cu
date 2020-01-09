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
#include "../finiteDifferences.h"
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
        float temperature = fmax(0,dist(rng));
        float velX = vdist(rng);
        float velY = vdist(rng);

        m_grid->write<AT::density>(i,density);
        m_grid->write<AT::temperature>(i,temperature);
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

    m_totalSimulatedTime = 0;
    return m_grid;
}

std::unique_ptr<Simulation> TestSimulation::clone() const
{
    return std::make_unique<TestSimulation>(*this);
}

void TestSimulation::showGui(bool* show)
{
    ImGui::SetNextWindowSize({0,0},ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Test Simulation",show))
    {
        if(m_isPaused)
        {
            ImGui::Text("State: Paused");
            if(ImGui::Button("Resume")) resume();
        }
        else
        {
            ImGui::Text("State: running");
            if(ImGui::Button("Pause")) pause();
        }

        ImGui::Checkbox("solve heat equation",&m_solveHeatEquation);
        ImGui::DragFloat("Heat Coefficient",&m_heatCoefficient,0.01);
        ImGui::DragFloat("Timestep",&m_timestep,0.01);
        ImGui::Text("Simulated Time units: %f", m_totalSimulatedTime);
    }
    ImGui::End();
}

void TestSimulation::simulateOnce()
{
    m_simOnceFunc(); // calls correct template specialization
}

template <typename csT>
__global__ void testSimulation(TestSimGrid::ReferenceType grid, csT cs, bool solveHeat, float heatCoefficient, float timestep)
{
    for(int x : mpu::gridStrideRange( 1, cs.getNumGridCells3d().x-1 ))
        for(int y : mpu::gridStrideRangeY( 1, cs.getNumGridCells3d().y-1 ))
    {
        int3 cell{x,y,0};
        int cellId = cs.getCellId(cell);

        float rho = grid.read<AT::density>(cellId);
        float velX = grid.read<AT::velocityX>(cellId);
        float velY = grid.read<AT::velocityY>(cellId);

        grid.write<AT::velocityX>(cellId, velX);
        grid.write<AT::velocityY>(cellId, velY);
        grid.write<AT::density>(cellId,rho);

        // calculate gradient using central difference
        // since we use the density at at i and i+1 we get the gradient halfway in between the cells,
        // on the edge between cell i and i+1
        float rhoRight     = grid.read<AT::density>(cs.getRightNeighbor(cellId));
        float rhoForward   = grid.read<AT::density>(cs.getForwardNeighbor(cellId));

        float2 gradRho = gradient2d(rho,rhoRight,rho,rhoForward, cs.getCellSize());

        grid.write<AT::densityGradX>(cellId, gradRho.x);
        grid.write<AT::densityGradY>(cellId, gradRho.y);

        // calculate divergence of the velocity field
        // remember, velocities are defined half way between the nodes,
        // we want the divergence at the node, so we get a central difference by looking at the velocities left and backwards from us
        // and compare them to our velocities
        float velLeftX = grid.read<AT::velocityX>(cs.getLeftNeighbor(cellId));
        float velBackwardY = grid.read<AT::velocityY>(cs.getBackwardNeighbor(cellId));

        float velDiv = divergence2d(velLeftX,velX,velBackwardY,velY,cs.getCellSize());

        grid.write<AT::velocityDiv>(cellId, velDiv);

        // laplace
        float rhoLeft     = grid.read<AT::density>(cs.getLeftNeighbor(cellId));
        float rhoBackward   = grid.read<AT::density>(cs.getBackwardNeighbor(cellId));

        float laplace = laplace2d(rhoLeft,rhoRight,rhoBackward,rhoForward,rho, cs.getCellSize());

        grid.write<AT::densityLaplace>(cellId, laplace);

        // curl is more difficult, as we can only compute it at cell corners
        // offsetted from where we want to visualize it
        // so we need to compute 4 curls and average them

        // forward right quadrant
        float velRightY = grid.read<AT::velocityY>(cs.getRightNeighbor(cellId));
        float velForwardX = grid.read<AT::velocityX>(cs.getForwardNeighbor(cellId));

        float forwardRightCurl = curl2d(velY,velRightY, velX, velForwardX, cs.getCellSize());
        // averaging is done in the next kernel
        grid.write<AT::velocityCurl>(cellId, forwardRightCurl);

        // solve the heat equation
        if(solveHeat)
        {
            float temp = grid.read<AT::temperature>(cellId);
            float tempLeft = grid.read<AT::temperature>(cs.getLeftNeighbor(cellId));
            float tempRight = grid.read<AT::temperature>(cs.getRightNeighbor(cellId));
            float tempForward = grid.read<AT::temperature>(cs.getForwardNeighbor(cellId));
            float tempBackward = grid.read<AT::temperature>(cs.getBackwardNeighbor(cellId));

            float temp_dt = heatCoefficient * laplace2d(tempLeft,tempRight,tempBackward,tempForward,temp,cs.getCellSize());
            temp += temp_dt * timestep;
            grid.write<AT::temperature>(cellId,temp);
        }
        else
            grid.copy<AT::temperature>(cellId);
    }
}

template <typename csT>
__global__ void interpolateCurl(TestSimGrid::ReferenceType grid, csT cs)
{
    for(int x : mpu::gridStrideRange( 1, cs.getNumGridCells3d().x-1 ))
        for(int y : mpu::gridStrideRangeY( 1, cs.getNumGridCells3d().y-1 ))
        {
            int3 cell{x,y,0};
            int cellId = cs.getCellId(cell);

            // only forward right curl was computed above, so now curl must be interpolated
            float curlForwardRight = grid.read<AT::velocityCurl>(cellId);
            float curlForwardLeft = grid.read<AT::velocityCurl>(cs.getLeftNeighbor(cellId));
            float curlBackwardsRight = grid.read<AT::velocityCurl>(cs.getBackwardNeighbor(cellId));
            float curlBackwardsLeft = grid.read<AT::velocityCurl>(cs.getLeftNeighbor(cs.getBackwardNeighbor(cellId)));

            float averageCurl = curlForwardRight + curlForwardLeft + curlBackwardsRight + curlBackwardsLeft;
            averageCurl *= 0.25;

            grid.write<AT::velocityCurl>(cellId, averageCurl);

            // copy all other values to the new buffer
            grid.copy<AT::velocityX>(cellId);
            grid.copy<AT::velocityY>(cellId);
            grid.copy<AT::density>(cellId);
            grid.copy<AT::densityGradX>(cellId);
            grid.copy<AT::densityGradY>(cellId);
            grid.copy<AT::densityLaplace>(cellId);
            grid.copy<AT::velocityDiv>(cellId);
            grid.copy<AT::temperature>(cellId);
        }
}

template <typename csT>
void TestSimulation::simulateOnceImpl(csT& cs)
{
    dim3 blocksize{16,16,1};
    dim3 numBlocks{ static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().x ,blocksize.x)),
                    static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().y ,blocksize.y)), 1};

    if(m_solveHeatEquation)
        m_totalSimulatedTime += m_timestep;
    testSimulation<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs,m_solveHeatEquation,m_heatCoefficient,m_timestep);
    m_grid->swapBuffer();
    interpolateCurl<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs);
}

template void TestSimulation::simulateOnceImpl<CartesianCoordinates2D>(CartesianCoordinates2D& cs);

GridBase& TestSimulation::getGrid()
{
    return *m_grid;
}
