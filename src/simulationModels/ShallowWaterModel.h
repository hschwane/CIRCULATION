/*
 * CIRCULATION
 * ShallowWaterModel.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the ShallowWaterModel class
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_SHALLOWWATERMODEL_H
#define CIRCULATION_SHALLOWWATERMODEL_H

// includes
//--------------------
#include "Simulation.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class ShallowWaterModel
 *
 * runs simulations using the shallow water equations
 *
 */
class ShallowWaterModel : public Simulation
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

    // creation settings
    float2 m_gaussianPosition{0,0}; //!< position of the gaussian disturbance
    float m_stdDev{0.1f}; //!< standard deviation of gaussian disturbance
    float m_multiplier{0.1f}; //!< value is multiplied with the gaussian

    // sim settings
    float m_timestep{0.0001}; //!< simulation timestep used
    bool m_useLeapfrog{true}; //!< should leapfrog be used
    float m_geopotDiffusion{0.0}; //!< diffusion amount
    float m_coriolisParameter{0.0}; //!< corrilois parameter for cartesian simulations
    float m_angularVelocity{7.2921e-5}; //!< angular velocity of earth

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs; //!< the coordinate system to be used
    std::shared_ptr<ShallowWaterGrid> m_grid; //!< the grid to be used
    mpu::DeviceVector<float> m_phiPlusKBuffer; //!< stores geopotential + kinetic energy
    mpu::DeviceVector<float> m_vortPlusCor; //!< stores vorticity + corriolis parameter
    float m_totalSimulatedTime{0.0f};
    bool m_firstTimestep{true};
};


#endif //CIRCULATION_SHALLOWWATERMODEL_H
