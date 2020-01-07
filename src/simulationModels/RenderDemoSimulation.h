/*
 * CIRCULATION
 * RenderDemoSimulation.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the RenderDemoSimulation class
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_RENDERDEMOSIMULATION_H
#define CIRCULATION_RENDERDEMOSIMULATION_H

// includes
//--------------------
#include "Simulation.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class RenderDemoSimulation
 *
 * usage:
 *
 */
class RenderDemoSimulation : public Simulation
{
public:
    void drawCreationOptions() override
    {
        ImGui::Checkbox("Random Vectors", &m_randomVectors);
        if(!m_randomVectors)
            ImGui::DragFloat2("Vector", &m_vectorValue.x);
    }

    std::shared_ptr<GridBase> recreate(std::shared_ptr<CoordinateSystem> cs) override
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

        return m_grid;
    }

    void run() override
    {
        // nothing to do, as this is only a rendering demo
    }

    void showGui(bool* show) override
    {
        ImGui::SetNextWindowSize({0,0},ImGuiCond_FirstUseEver);
        if(ImGui::Begin("RenderDemoSimulation",show))
        {
            ImGui::Text("The render demo does not have any simulation settings");
        }
        ImGui::End();
    }

private:
    // creation options
    bool m_randomVectors{true};
    float2 m_vectorValue;

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs;
    std::shared_ptr<RenderDemoGrid> m_grid;

};


#endif //CIRCULATION_RENDERDEMOSIMULATION_H
