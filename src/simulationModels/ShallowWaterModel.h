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

    // sim data
    std::shared_ptr<CoordinateSystem> m_cs; //!< the coordinate system to be used
    std::shared_ptr<TestSimGrid> m_grid; //!< the grid to be used

};


#endif //CIRCULATION_SHALLOWWATERMODEL_H
