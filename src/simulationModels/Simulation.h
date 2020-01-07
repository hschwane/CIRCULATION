/*
 * CIRCULATION
 * Simulation.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the Simulation class
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_SIMULATION_H
#define CIRCULATION_SIMULATION_H

// includes
//--------------------
#include <memory>

#include <mpUtils/mpUtils.h>
#include <mpUtils/mpGraphics.h>
#include <mpUtils/mpCuda.h>

#include "../Grid.h"
#include "../coordinateSystems/CoordinateSystem.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class Simulation
 *
 * Base class for different simulation models.
 * Used to control and run different simulations.
 *
 */
class Simulation
{
public:
    virtual ~Simulation()=default;

    // creation
    virtual void drawCreationOptions()=0; //!< draws part of a ui window that enables changing of options in the "create new simulation"-dialog
    virtual std::shared_ptr<GridBase> recreate(std::shared_ptr<CoordinateSystem> cs)=0; //!< recreate simulation using current creation options, returns new coordinate system

    // running the simulation
    virtual void run()=0; //!< runs simulation
    virtual void showGui(bool* show)=0; //!< show user interface for simulation
};


#endif //CIRCULATION_SIMULATION_H
