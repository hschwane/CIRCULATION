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
    virtual std::unique_ptr<Simulation> clone() const =0; //!< deep copy of the simulation

    // running the simulation
    void run(int iterations=1); //!< runs simulation for iteration timesteps, does nothing if simulation is paused
    virtual void showGui(bool* show)=0; //!< show user interface for simulation
    void pause() {m_isPaused=true;} //!< pauses the simulation
    void resume() {m_isPaused=false;} //!< resumes the simulation
    bool isPaused() {return m_isPaused;} //!< checks if the simulation should be paused

protected:
    bool m_isPaused{false};

private:
    virtual void simulateOnce()=0; //!< simulate one timestep
    virtual GridBase& getGrid()=0; //!< access to the simulation grid
};

inline void Simulation::run(int iterations)
{
    if(m_isPaused)
        return;

    for(int i=0; i<iterations; i++)
        simulateOnce();
}


#endif //CIRCULATION_SIMULATION_H
