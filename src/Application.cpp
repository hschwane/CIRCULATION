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
    : m_window(width,height,"CIRCULATION")
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
    m_window.addFBSizeCallback([&](int w, int h)
                             {
                                 glViewport(0,0,w,h);
                                 this->m_width = w;
                                 this->m_width = h;
                                 this->m_aspect = m_width / m_height;
                             });
}

bool Application::run()
{
    mpu::gph::Input::update();
    if( !m_window.frameBegin())
        return false;

    ImGui::ShowDemoWindow();

    m_window.frameEnd();
    return true;
}
