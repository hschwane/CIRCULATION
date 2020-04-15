/*
 * CIRCULATION
 * Application.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the Application class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_APPLICATION_H
#define CIRCULATION_APPLICATION_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpGraphics.h>
#include <mpUtils/mpCuda.h>

#include "globalSettings.h"
#include "Renderer.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class Application
 *
 * usage:
 * Create an application with the initial window size and call run() in a loop until it returns false.
 *
 */
class Application
{
public:
    Application(int width, int height);
    ~Application();
    bool run(); //!< returns false when app should be closed

private:

    // window management
    mpu::gph::Window m_window; //!< main window
    int m_width; //!< main window framebuffer width
    int m_height; //!< main window framebuffer height
    bool m_vsync{true}; //!< is vsync enabled?

    // initial conditions
    void constructIcosphere();

    // gl buffer for simulation data
    std::vector<float2> m_geoPointsCPU;
    std::vector<float3> m_cartPointsCPU;
    std::vector<GLuint> m_triIndicesCPU;
    mpu::gph::Buffer<float3> m_cartPos;
    mpu::gph::Buffer<float2> m_geoPos;
    mpu::gph::Buffer<GLuint> m_triangleIndices;
    mpu::gph::Buffer<float2,true> m_velocityBuffer;
    mpu::gph::Buffer<float,true> m_geopotentialBuffer;

    // simulation settings

    // internal units
    float m_earthRadiusSI{6.37122e6}; //!< mean radius of earth (m)
    float m_lengthUnit; //!< internal length unit (depends on radius of earth)
    float m_timeUnit{60*60*24}; //!< internal time unit
    float m_gSI{9.80616}; //!< gravitational acceleration in m/s^2
    float m_g; //!< gravitational acceleration in internal units

    // general settings
    float m_timestep{0.0001}; //!< simulation timestep used
    int m_simulationStepsPerFrame{10}; //!< simulation steps done before each rendering
    float m_totalSimulatedTime{0.0f}; //!< internal time since beginning of the simulation

    // cosine advection simulation
    // SI units
    float m_u0SI{2*M_PI*6.37122e6/1036800}; //!< advection wind velocity (m/s)
    float m_angularVelocitySI{7.2921e-5}; //!< angular velocity of earth radiants/s
    float m_cosineBellRadiusSI{6.37122e6/3}; //!< cosine bell size in meter
    float m_h0SI{1000}; //!< initial height of cosine bell in meter

    // internal units
    float2 m_cosineBellCenter{0,0.5};//3*M_PI/2}; //!< center of the cosine bell
    float m_u0; //!< advection wind velocity (lengthUnit / timeUnit)
    float m_angularVelocity; //!< angular velocity of earth (radians / time unit)
    float m_cosineBellRadius; //!< cosine bell size (lengthUnit)
    float m_h0; //!< initial height of cosine bell in lengthUnits
    float m_alpha{0.0f}; //!< alpha parameter (radians)

    // rendering
    Renderer m_renderer; //!< the renderer class
    mpu::gph::Camera m_camera; //!< the camera used by the renderer to draw results

    // user interface
    bool m_showImGuiDemoWindow{false}; //!< is true ImGUI demo window will be shown
    bool m_showCameraDebugWindow{false}; //!< if true camera debug window will be drawn
    bool m_showPerfWindow{false}; //!< if true performance window will be drawn
    bool m_showAboutWindow{false}; //!< if true about window will be drawn
    bool m_showKeybindingsWindow{false}; //!< if true keybinding window will be drawn
    bool m_showRendererWindow{false}; //!< if true renderer window will be drawn
    bool m_showSimulationWindow{false}; //!< if true the simulation settings window will be drawn

    // internal helper functions
    void addInputs(); //!< add some useful input functions
    void setKeybindings(); //!< set keybindings for all the functions
    void resetCamera(); //!< resets the camera

    // simulation
    void resetSimulation(); // resets the simulation to initial conditions
    void simulate(); // run the simulation for some time
    void simulateOnce(); // run one timestep of the simulation

    // ui windows and menus
    void mainMenuBar(); //!< draw and handle the main menu bar
    void showPerfWindow(bool* show); //!< shows window with performance information and settings
    void showAboutWindow(bool* show); //!< shows window with information on app
    void showKeybindingsWindow(bool* show); //!< shows window with information keybindings
    void newSimulationModal(); //!< handles the new simulation model

    mpu::CfgFile m_persist;

    // simulation creation settings
    int m_n{10}; // number of cells per thrombus
};


#endif //CIRCULATION_APPLICATION_H
