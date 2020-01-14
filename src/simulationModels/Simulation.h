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
    virtual void showCreationOptions()=0; //!< draws part of a ui window that enables changing of options in the "create new simulation"-dialog
    virtual void showBoundaryOptions(const CoordinateSystem& cs)=0; //!< draws part of a ui window that enables changing boundary conditions
    virtual std::shared_ptr<GridBase> recreate(std::shared_ptr<CoordinateSystem> cs)=0; //!< recreate simulation using current creation options, returns new coordinate system, feel free to call reset() here
    virtual void reset()=0; //!< reset the simulation to the initial conditions, keep allocated memory and settings
    virtual std::unique_ptr<Simulation> clone() const =0; //!< deep copy of the simulation

    // running the simulation
    void run(int iterations=1); //!< runs simulation for iteration timesteps, does nothing if simulation is paused
    void showGui(bool* show); //!< show user interface for simulation
    void pause() {m_isPaused=true;} //!< pauses the simulation
    void resume() {m_isPaused=false;} //!< resumes the simulation
    bool isPaused() {return m_isPaused;} //!< checks if the simulation should be paused

protected:
    bool m_isPaused{false};

private:
    virtual void showSimulationOptions()=0; //!< draws part of a ui window to handle all live settings that can be changed while the simulation is running if you want you can call "showBoundaryOptions" here as well
    virtual void simulateOnce()=0; //!< simulate one timestep
    virtual GridBase& getGrid()=0; //!< access to the simulation grid
    virtual std::string getDisplayName()=0; //!< name of the simulation to be displayed in the ui
};

inline void Simulation::run(int iterations)
{
    if(m_isPaused)
        return;

    // simulate all iterations but one
    for(int i=0; i<iterations-1; i++)
    {
        simulateOnce();
        getGrid().swapBuffer();
    }

    // simulate the final iteration
    simulateOnce();
    getGrid().swapAndRender();
}

inline void Simulation::showGui(bool* show)
{
    ImGui::SetNextWindowSize({0,0},ImGuiCond_FirstUseEver);
    if(ImGui::Begin(getDisplayName().c_str(), show))
    {
        if(m_isPaused)
        {
            ImGui::Text("State: Paused");
            if(ImGui::Button("Resume")) resume();
        }
        else
        {
            ImGui::Text("State: running");
            if(ImGui::Button("Pause")) pause();
        }
        ImGui::SameLine();
        if( ImGui::Button("Reset")) reset();

        ImGui::Separator();
        showSimulationOptions();
    }
    ImGui::End();
}


#endif //CIRCULATION_SIMULATION_H
