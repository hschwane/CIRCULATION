/*
 * CIRCULATION
 * Application.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the Application class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "Application.h"
//--------------------

// function definitions of the Application class
//-------------------------------------------------------------------
Application::Application(int width, int height)
    : m_window(width,height,"CIRCULATION"),
    m_camera(mpu::gph::Camera::trackball, glm::vec3(0,0,2), glm::vec3(0,0,0))
{
    // add shader include pathes
    mpu::gph::addShaderIncludePath(MPU_LIB_SHADER_PATH"include");
    mpu::gph::addShaderIncludePath(PROJECT_SHADER_PATH"include");

    // setup GUI
    ImGui::create(m_window);

    // some gl settings
    mpu::gph::enableVsync(true);
    glClearColor( .2f, .2f, .2f, 1.0f);

    // add resize callback
    m_window.addFBSizeCallback([this](int w, int h)
                             {
                                 glViewport(0,0,w,h);
                                 this->m_width = w;
                                 this->m_width = h;
                                 this->m_aspect = m_width / m_height;
                             });

    // add input functions
    m_camera.addInputs();
    addInputs();
}

bool Application::run()
{
    mpu::gph::Input::update();
    if( !m_window.frameBegin())
        return false;

    ImGui::ShowDemoWindow();
    m_camera.showDebugWindow();

    m_camera.update();

    m_window.frameEnd();
    return true;
}

void Application::addInputs()
{
    using namespace mpu::gph;
    // close app on escape
    Input::addButton("Close", "close application",
                          [](Window& wnd) { wnd.shouldClose(); });

    // fullscreen app on F11
    Input::addButton("ToggleFullscreen","switch between fullscreen and windowed mode",
                          [](Window& wnd) { wnd.toggleFullscreen(); });

    Input::addButton("ResetCamera", "reset the camera based on loaded grid",
            [this](Window&) { this->resetCamera(); });
}

void Application::resetCamera()
{
    m_camera.setPosition({0,0,2});
    m_camera.setTarget({0,0,0});
}
