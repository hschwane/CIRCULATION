/*
 * CIRCULATION
 * TestSimulation.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the TestSimulation class
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_TESTSIMULATION_H
#define CIRCULATION_TESTSIMULATION_H

// includes
//--------------------
#include "Simulation.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class TestSimulation
 *
 * usage:
 *
 */
class TestSimulation : public Simulation
{
public:
    void drawCreationOptions() override;
    std::shared_ptr<GridBase> recreate(std::shared_ptr<CoordinateSystem> cs) override;
    std::unique_ptr<Simulation> clone() const override;
    void showGui(bool* show) override;

private:
    void simulateOnce() override;
    GridBase& getGrid() override;

    // creation options
    bool m_randomVectors{true};
    float2 m_vectorValue;

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs;
    std::shared_ptr<RenderDemoGrid> m_grid;
};


#endif //CIRCULATION_TESTSIMULATION_H
