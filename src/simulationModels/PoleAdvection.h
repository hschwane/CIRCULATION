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

#ifndef CIRCULATION_POLEADVECTION_H
#define CIRCULATION_POLEADVECTION_H

// includes
//--------------------
#include "Simulation.h"
#include "../coordinateSystems/GeographicalCoordinates2D.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class ShallowWaterModel
 *
 * runs simulations using the shallow water equations
 *
 */
class PoleAdvection : public Simulation
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

    void simulateOnceImpl(GeographicalCoordinates2D& cs); //!< implementation of simulate once

    // creation settings
    float m_alpha; //!< alpha parameter
    float m_earthRadius{6.37122e6}; //!< mean radius of earth
    float m_u0{2*M_PI*6.37122e6 / 1036800}; //!< advectiong wind velocity

    // sim settings
    float m_angularVelocity{7.2921e-5}; //!< angular velocity of earth
    float m_timestep{0.0001}; //!< simulation timestep used
    bool m_useLeapfrog{true}; //!< should leapfrog be used
    float m_geopotDiffusion{0.0}; //!< diffusion amount

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs; //!< the coordinate system to be used
    std::shared_ptr<ShallowWaterGrid> m_grid; //!< the grid to be used
    mpu::DeviceVector<float> m_phiPlusKBuffer; //!< stores geopotential + kinetic energy
    mpu::DeviceVector<float> m_vortPlusCor; //!< stores vorticity + corriolis parameter
    float m_totalSimulatedTime{0.0f};
    bool m_firstTimestep{true};
};


#endif //CIRCULATION_POLEADVECTION_H
