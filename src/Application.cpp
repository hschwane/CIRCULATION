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
    mpu::gph::enableVsync(m_vsync);
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

    // -------------------------
    // handle user interface
    // draw main menu
    mainMenuBar();

    // draw windows if needed
    if(m_showImGuiDemoWindow) ImGui::ShowDemoWindow(&m_showImGuiDemoWindow);
    if(m_showCameraDebugWindow) m_camera.showDebugWindow(&m_showCameraDebugWindow);
    if(m_showPerfWindow) showPerfWindow(m_showPerfWindow);
    if(m_showAboutWindow)
        showAboutWindow(m_showAboutWindow);

    // -------------------------
    // simulation

    // -------------------------
    // rendering

    // update camera
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

    // hde gui for nice screenshots
    Input::addButton("ToggleGUI","toggle visibility the user interface", [this](Window&){ImGui::toggleVisibility();});
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
    Input::mapKeyToInput("ToggleGUI", GLFW_KEY_TAB);
}

void Application::resetCamera()
{
    m_camera.setPosition({0,0,2});
    m_camera.setTarget({0,0,0});
}

void Application::mainMenuBar()
{
    if(ImGui::BeginMainMenuBar())
    {
        // window menu to select shown windows
        if(ImGui::BeginMenu("Windows"))
        {
            ImGui::MenuItem("performance", nullptr, &m_showPerfWindow);
            ImGui::MenuItem("camera debug window", nullptr, &m_showCameraDebugWindow);
            ImGui::MenuItem("ImGui demo window", nullptr, &m_showImGuiDemoWindow);

            ImGui::EndMenu();
        }

        // window menu to select shown windows
        if(ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("About", nullptr, &m_showAboutWindow);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Application::showPerfWindow(bool &show)
{
    if(ImGui::Begin("performance",&show))
    {
        ImGui::Text("Frametime: %f", mpu::gph::Input::deltaTime());
        ImGui::Text("FPS: %f", 1.0f / mpu::gph::Input::deltaTime());

        if(ImGui::Checkbox("V-Sync",&m_vsync))
            mpu::gph::enableVsync(m_vsync);
    }
}

void Application::showAboutWindow(bool& show)
{
    ImGui::SetNextWindowSize({500,0});
    if(ImGui::Begin("About",&show, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("CIRCULATION");
        ImGui::Text("Cuda Inderactive Climate simULATION");
        ImGui::Text("Developed by Hendrik Schwanekamp\nhendrik.schwanekamp@gmx.net");
        ImGui::Text("on Gituhb:\n https://github.com/hschwane/CIRCULATION");

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::TextWrapped("Included third party software:\n\n "
                    "GCE-Math: A C++ generalized constant expression-based math library Copyright 2016-2019 Keith O'Hara This product includes software developed by Keith O'Hara (http://www.kthohr.com)\n"
                    "\n"
                    "This software contains source code provided by NVIDIA Corporation.\n\n"
                    "CUB by nvlabs (https://nvlabs.github.io/cub/)\n"
                    "\n"
                    "stb_image (https://github.com/nothings/stb) This software contains source code provided by Sean T. Barrett.\n"
                    "\n"
                    "Dear ImGui (https://github.com/ocornut/imgui) This software contains source code provided by Omar Cornut.\n"
                    "\n"
                    "tiny file dialogs (ysengrin.com) This software contains source code provided by Guillaume Vareille.\n"
                    "\n"
                    "Test textures by Thomas Schmall (https://www.oxpal.com/uv-checker-texture.html)\n"
                    "\n"
                    "GLShader by Johannes Braun (https://github.com/johannes-braun/GLshader)  \n");

        if(ImGui::Button("Close"))
            show = false;
    }
}
