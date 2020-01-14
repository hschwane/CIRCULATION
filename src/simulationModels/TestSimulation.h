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
 * A simulation for testing finite difference approximations
 *
 */
class TestSimulation : public Simulation
{
public:
    void showCreationOptions() override;
    void showBoundaryOptions(const CoordinateSystem& cs) override;

    std::shared_ptr<GridBase> recreate(std::shared_ptr<CoordinateSystem> cs) override;
    void reset() override;
    std::unique_ptr<Simulation> clone() const override;

private:
    void showSimulationOptions() override;
    void simulateOnce() override;
    GridBase& getGrid() override;
    std::string getDisplayName() override;

    template <typename csT>
    void simulateOnceImpl(csT& cs); //!< implementation of simulate once to allow different coordinate systems to be used
    std::function<void()> m_simOnceFunc; //!< will be set to use the correct template specialisation based on type of coordinate system used

    // creation options
    bool m_randomVectors{true};
    float2 m_vectorValue;

    // boundary settings
    bool m_boundaryIsolatedX{false};
    float m_boundaryTemperatureX{6.0f};
    bool m_boundaryIsolatedY{false};
    float m_boundaryTemperatureY{6.0f};

    // sim options
    bool m_diffuseHeat{false};
    bool m_advectHeat{false};
    float m_heatCoefficient{0.01f};
    float m_timestep{0.001f}; // 0.006
    float m_totalSimulatedTime{0.0f};
    bool m_useDivOfGrad{false};
    bool m_leapfrogIntegrattion{false};

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs; //!< the coordinate system to be used
    std::shared_ptr<TestSimGrid> m_grid; //!< the grid to be used

    mpu::DeviceVector<float> m_offsettedCurl; //!< offsetted curl is moved from kernel A to kernel B using this buffer
    bool m_firstTimestep{true};
    bool m_needUpdateBoundaries{false};
};


#endif //CIRCULATION_TESTSIMULATION_H
