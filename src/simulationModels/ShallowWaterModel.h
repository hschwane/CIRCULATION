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

    // sim settings
    float m_timestep{0.00001}; //!< simulation timestep used

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs; //!< the coordinate system to be used
    std::shared_ptr<ShallowWaterGrid> m_grid; //!< the grid to be used
    mpu::DeviceVector<float> m_phiPlusKBuffer; //!< stores geopotential + kinetic energy
    float m_totalSimulatedTime{0.0f};
};


#endif //CIRCULATION_SHALLOWWATERMODEL_H
