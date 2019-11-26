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
                                 logDEBUG("Application") << "window resized. w " << w << " h " << h;
                                 this->m_aspect = float(m_width) / float(m_height);
                             });

    // add input functions
    m_camera.addInputs();
    addInputs();
    setKeybindings();
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

    // ability to reset the camera
    Input::addButton("ResetCamera", "reset the camera based on loaded grid",
            [this](Window&) { this->resetCamera(); });
}

void Application::setKeybindings()
{
    using namespace mpu::gph;

    // camera
    Input::mapKeyToInput("CameraMoveSideways",GLFW_KEY_D,Input::ButtonBehavior::whenDown,Input::AxisBehavior::positive);
    Input::mapKeyToInput("CameraMoveSideways",GLFW_KEY_A,Input::ButtonBehavior::whenDown,Input::AxisBehavior::negative);
    Input::mapKeyToInput("CameraMoveForwardBackward",GLFW_KEY_W,Input::ButtonBehavior::whenDown,Input::AxisBehavior::positive);
    Input::mapKeyToInput("CameraMoveForwardBackward",GLFW_KEY_S,Input::ButtonBehavior::whenDown,Input::AxisBehavior::negative);
    Input::mapKeyToInput("CameraMoveUpDown",GLFW_KEY_Q,Input::ButtonBehavior::whenDown,Input::AxisBehavior::negative);
    Input::mapKeyToInput("CameraMoveUpDown",GLFW_KEY_E,Input::ButtonBehavior::whenDown,Input::AxisBehavior::positive);

    Input::mapCourserToInput("CameraPanHorizontal", Input::AxisOrientation::horizontal,Input::AxisBehavior::negative,0, "EnablePan");
    Input::mapCourserToInput("CameraPanVertical", Input::AxisOrientation::vertical,Input::AxisBehavior::positive,0, "EnablePan");
    Input::mapScrollToInput("CameraZoom");

    Input::mapMouseButtonToInput("EnablePan", GLFW_MOUSE_BUTTON_MIDDLE);
    Input::mapKeyToInput("EnablePan", GLFW_KEY_LEFT_ALT);

    Input::mapCourserToInput("CameraRotateHorizontal", Input::AxisOrientation::horizontal,Input::AxisBehavior::negative,0, "EnableRotation");
    Input::mapCourserToInput("CameraRotateVertical", Input::AxisOrientation::vertical,Input::AxisBehavior::negative,0, "EnableRotation");

    Input::mapMouseButtonToInput("EnableRotation", GLFW_MOUSE_BUTTON_LEFT);
    Input::mapKeyToInput("EnableRotation", GLFW_KEY_LEFT_CONTROL);

    Input::mapKeyToInput("CameraMovementSpeed",GLFW_KEY_RIGHT_BRACKET,Input::ButtonBehavior::whenDown,Input::AxisBehavior::positive);
    Input::mapKeyToInput("CameraMovementSpeed",GLFW_KEY_SLASH,Input::ButtonBehavior::whenDown,Input::AxisBehavior::negative);
    Input::mapKeyToInput("CameraToggleMode",GLFW_KEY_R);
    Input::mapKeyToInput("CameraSlowMode",GLFW_KEY_LEFT_SHIFT,Input::ButtonBehavior::whenDown);
    Input::mapKeyToInput("CameraFastMode",GLFW_KEY_SPACE,Input::ButtonBehavior::whenDown);

    // generic
    Input::mapKeyToInput("Close",GLFW_KEY_ESCAPE);
    Input::mapKeyToInput("ToggleFullscreen",GLFW_KEY_F11);
    Input::mapKeyToInput("ResetCamera",GLFW_KEY_X);
}

void Application::resetCamera()
{
    m_camera.setPosition({0,0,2});
    m_camera.setTarget({0,0,0});
}
