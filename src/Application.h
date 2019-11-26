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
    bool run(); //!< returns false when app should be closed

private:

    // window management
    mpu::gph::Window m_window; //!< main window
    int m_width; //!< main window framebuffer width
    int m_height; //!< main window framebuffer height
    float m_aspect; //!< main window framebuffer aspect ratio

    // camera
    mpu::gph::Camera m_camera; //!< the camera used by the renderer to draw results

    // user interface
    bool m_showImGuiDemoWindow; //!< is true ImGUI demo window will be shown
    bool m_showCameraDebugWindow{false}; //!< if true camera debug window will be drawn

    // internal helper functions
    void addInputs(); //!< add some useful input functions
    void setKeybindings(); //!< set keybindings for all the functions
    void resetCamera(); //!< resets the camera

    void mainMenuBar(); //!< draw and handle the main menu bar
};


#endif //CIRCULATION_APPLICATION_H
