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

#ifndef CIRCULATION_COSINEADVECTION_H
#define CIRCULATION_COSINEADVECTION_H

// includes
//--------------------
#include "Simulation.h"
#include "../coordinateSystems/GeographicalCoordinates2D.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class CosineAdvection
 *
 * Test Case number 1 from David L. Williamson 1992.
 *
 */
class CosineAdvection : public Simulation
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

    void buildWindField(); //!< set a new velocity field

    // settings in SI units
    float m_earthRadiusSI{6.37122e6}; //!< mean radius of earth (m)
    float m_u0SI{2*M_PI*6.37122e6/1036800}; //!< advection wind velocity (m/s)
    float m_angularVelocitySI{7.2921e-5}; //!< angular velocity of earth radiants/s
    float m_cosineBellRadiusSI{6.37122e6/3}; //!< cosine bell size in meter
    float m_h0SI{1000}; //!< initial height of cosine bell in meter
    float m_gSI{9.80616}; //!< gravitational acceleration in m/s^2

    float m_timeUnit{60*60*24}; //!< internal time unit
    float m_lengthUnit; //!< internal length unit (depends on cs size)

    // settings in internal units
    float2 m_cosineBellCenter{3*M_PI/2,1.2}; //!< center of the cosine bell
    float m_earthRadius; //!< advection wind velocity (lengthUnit / timeUnit)
    float m_u0; //!< advection wind velocity (lengthUnit / timeUnit)
    float m_angularVelocity; //!< angular velocity of earth (radians / time unit)
    float m_cosineBellRadius; //!< cosine bell size (lengthUnit)
    float m_h0; //!< initial height of cosine bell in meter
    float m_g; //!< gravitational acceleration in m/s^2

    float m_alpha{1.571}; //!< alpha parameter (radians)

    // sim settings
    float m_timestep{0.0001}; //!< simulation timestep used
    bool m_useLeapfrog{true}; //!< should leapfrog be used

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs; //!< the coordinate system to be used
    std::shared_ptr<ShallowWaterGrid> m_grid; //!< the grid to be used
    float m_totalSimulatedTime{0.0f};
    bool m_firstTimestep{true};
};


#endif //CIRCULATION_COSINEADVECTION_H
