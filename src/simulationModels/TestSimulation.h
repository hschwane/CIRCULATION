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

    template <typename csT>
    void simulateOnceImpl(csT& cs); //!< implementation of simulate once to allow different coordinate systems to be used
    std::function<void()> m_simOnceFunc; //!< will be set to use the correct template specialisation based on type of coordinate system used

    // creation options
    bool m_randomVectors{true};
    float2 m_vectorValue;

    // sim options
    bool m_solveHeatEquation{false};
    float m_heatCoefficient{1.0f};
    float m_timestep{0.1f};
    float m_totalSimulatedTime{0.0f};

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs; //!< the coordinate system to be used
    std::shared_ptr<TestSimGrid> m_grid; //!< the grid to be used
};


#endif //CIRCULATION_TESTSIMULATION_H
