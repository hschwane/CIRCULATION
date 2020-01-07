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
    m_grid = std::make_shared<RenderDemoGrid>(m_cs->getNumGridCells());

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

template <typename csT>
__global__ void testSimulation(RenderDemoGrid::ReferenceType grid, csT cs)
{
    printf("dim: %i\n",cs.getDimension());
    for(int i : mpu::gridStrideRange(grid.size()))
    {
        float rho = grid.read<AT::density>(i);
        grid.write<AT::density>(i,rho);
    }
}


template <typename csT>
void TestSimulation::simulateOnceImpl(csT& cs)
{
    testSimulation<<<mpu::numBlocks(m_grid->size(),128),128>>>(m_grid->getGridReference(),cs);
    cudaDeviceSynchronize();
}

template void TestSimulation::simulateOnceImpl<CartesianCoordinates2D>(CartesianCoordinates2D& cs);

GridBase& TestSimulation::getGrid()
{
    return *m_grid;
}
