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
#include <random>
//--------------------

// function definitions of the Application class
//-------------------------------------------------------------------
Application::Application(int width, int height)
    : m_window(width,height,"CIRCULATION"),
    m_camera(mpu::gph::Camera::trackball, glm::vec3(0,0,2), glm::vec3(0,0,0),glm::vec3(0,0,1))
{
    // setup GUI
    ImGui::create(m_window);

    // some gl settings
    mpu::gph::enableVsync(m_vsync);

    // add resize callback
    m_window.addFBSizeCallback([this](int w, int h)
                             {
                                 glViewport(0,0,w,h);
                                 this->m_width = w;
                                 this->m_width = h;
//                                 this->m_renderer.setSize(w,h);
                                 this->m_aspect = float(m_width) / float(m_height);
                             });

    // add input functions
    m_camera.setZoomSpeed(0.2);
    m_camera.addInputs();
    addInputs();
    setKeybindings();

    // try to load existing settings
    try {
        m_persist.open(persistFilename);
        m_showImGuiDemoWindow = m_persist.getValue<bool>("ui_windows", "ImGuiDemo");
        m_showCameraDebugWindow = m_persist.getValue<bool>("ui_windows", "cameraWindow");
        m_showPerfWindow = m_persist.getValue<bool>("ui_windows", "perfWindow");
        m_showAboutWindow = m_persist.getValue<bool>("ui_windows", "aboutWindow");
        m_showKeybindingsWindow = m_persist.getValue<bool>("ui_windows", "keybindingWindow");
        m_showRendererWindow = m_persist.getValue<bool>("ui_windows", "renderWindow");
        m_showSimulationWindow = m_persist.getValue<bool>("ui_windows", "simulateWindow");
        m_window.setSize( m_persist.getValue<int>("window", "width"), m_persist.getValue<int>("window", "height"));
    }
    catch (const std::exception& e)
    {
        logWARNING("Application") << "Could not load persistence file.";
        try {
            m_persist.createAndOpen(persistFilename);
        }
        catch (const std::exception& e)
        {
            logWARNING("Application") << "Could not create new persistence file.";
        }
    }
}

Application::~Application()
{
    // try to load existing settings
    try {
        auto s = m_window.getSize();
        m_persist.setValue<int>("window", "width", s.x);
        m_persist.setValue<int>("window", "height", s.y);
        m_persist.setValue<bool>("ui_windows", "ImGuiDemo", m_showImGuiDemoWindow);
        m_persist.setValue<bool>("ui_windows", "cameraWindow", m_showCameraDebugWindow);
        m_persist.setValue<bool>("ui_windows", "perfWindow", m_showPerfWindow);
        m_persist.setValue<bool>("ui_windows", "aboutWindow", m_showAboutWindow);
        m_persist.setValue<bool>("ui_windows", "keybindingWindow", m_showKeybindingsWindow);
        m_persist.setValue<bool>("ui_windows", "renderWindow", m_showRendererWindow);
        m_persist.setValue<bool>("ui_windows", "simulateWindow", m_showSimulationWindow);
    }
    catch (const std::exception& e)
    {
        logWARNING("Application") << "Could not store settings to persistence file.";
    }
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
    if(m_showPerfWindow) showPerfWindow(&m_showPerfWindow);
    if(m_showAboutWindow) showAboutWindow(&m_showAboutWindow);
    if(m_showKeybindingsWindow) showKeybindingsWindow(&m_showKeybindingsWindow);
//    if(m_showRendererWindow) m_renderer.showGui(&m_showRendererWindow);
//    if(m_showSimulationWindow && m_simulation != nullptr) m_simulation->showGui(&m_showSimulationWindow);

    // open new simulation modal on startup
//    static struct Once{Once(){ImGui::OpenPopup("New Simulation");}}once;
//    newSimulationModal();

    // -------------------------
    // simulation
//    if(m_simulation)
//        m_simulation->run();

    // -------------------------
    // rendering
    m_camera.update();
//    m_renderer.setViewMat(m_camera.viewMatrix());
//    if(m_grid)
//    {
//        m_grid->startRendering();
//        m_renderer.draw();
//        m_grid->renderDone();
//    }

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
    Input::addButton("ToggleGUI","toggle visibility the user interface", [](Window&){ImGui::toggleVisibility();});

    // add buttons to pause and resume
//    Input::addButton("Pause", "Pause the simulation",  [this](Window& wnd) { if(this->m_simulation)this->m_simulation->pause(); });
//    Input::addButton("Resume", "Resume the simulation",  [this](Window& wnd) { if(this->m_simulation)this->m_simulation->resume(); });
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

    // simulation
    Input::mapKeyToInput("Resume",GLFW_KEY_1);
    Input::mapKeyToInput("Pause",GLFW_KEY_2);
}

void Application::resetCamera()
{
//    glm::vec3 aabbMin{m_cs->getAABBMin().x, m_cs->getAABBMin().y, m_cs->getAABBMin().z};
//    glm::vec3 aabbMax{m_cs->getAABBMax().x, m_cs->getAABBMax().y, m_cs->getAABBMax().z};

//    glm::vec3 size = aabbMax - aabbMin;
//    float diagonal = glm::length(size);
//    glm::vec3 center = aabbMin + size/2;

//    m_camera.setPosition(glm::vec3(0,-0.75*diagonal,diagonal));
//    m_camera.setTarget(center);
}

void Application::mainMenuBar()
{
    if(ImGui::BeginMainMenuBar())
    {
        // simulation menu to manage the simulation
        if(ImGui::BeginMenu("Simulation"))
        {
//            if(ImGui::MenuItem("New"))
//                newSimPressed=true; // needed for some imGui id stack thing
//
//            // disable menue in case simulation is not valid
//            if(!m_simulation)
//            {
//                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
//                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
//            }
//
//            ImGui::Separator();
//
//            if(m_simulation && m_simulation->isPaused())
//            {
//                if(ImGui::MenuItem("Resume","1"))
//                    m_simulation->resume();
//            }
//            else
//            {
//                if(ImGui::MenuItem("Pause","2"))
//                    m_simulation->pause();
//            }
//
//            if(ImGui::MenuItem("Reset"))
//            {
//                m_grid = m_simulation->recreate(m_cs);
//                m_grid->addRenderBufferToVao(m_renderer.getVAO(), 0);
//                m_grid->bindRenderBuffer(0, GL_SHADER_STORAGE_BUFFER);
//            }
//
//            ImGui::Separator();
//
//            ImGui::MenuItem("Show Simulation window", nullptr, &m_showSimulationWindow);
//
//            if(!m_simulation)
//            {
//                ImGui::PopItemFlag();
//                ImGui::PopStyleVar();
//            }
//
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Visualization"))
        {
            if(ImGui::MenuItem("Reset Camera","X"))
                resetCamera();

            if(ImGui::MenuItem("Toggle Camera Mode","R"))
                m_camera.toggleMode();

            ImGui::Separator();
//            ImGui::MenuItem("Show Visualization window", nullptr, &m_showRendererWindow);

            ImGui::EndMenu();
        }

        // window menu to select shown windows
        if(ImGui::BeginMenu("Windows"))
        {
            ImGui::MenuItem("performance", nullptr, &m_showPerfWindow);
//            ImGui::MenuItem("visualization", nullptr, &m_showRendererWindow);
//            ImGui::MenuItem("simulation", nullptr, &m_showSimulationWindow);
            ImGui::MenuItem("camera debug window", nullptr, &m_showCameraDebugWindow);
            ImGui::Separator();
            ImGui::MenuItem("ImGui demo window", nullptr, &m_showImGuiDemoWindow);
            ImGui::EndMenu();
        }

        // window menu to select shown windows
        if(ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("Keybindings", nullptr, &m_showKeybindingsWindow);
            ImGui::MenuItem("About", nullptr, &m_showAboutWindow);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Application::showPerfWindow(bool* show)
{
    ImGui::SetNextWindowSize({0,0},ImGuiCond_FirstUseEver);
    if(ImGui::Begin("performance",show))
    {
        ImGui::Text("Frametime: %f", mpu::gph::Input::deltaTime());
        ImGui::Text("FPS: %f", 1.0f / mpu::gph::Input::deltaTime());

        if(ImGui::Checkbox("V-Sync",&m_vsync))
            mpu::gph::enableVsync(m_vsync);
    }
    ImGui::End();
}

void Application::showAboutWindow(bool* show)
{
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
                            ImGuiCond_Appearing, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize({500,0});
    if(ImGui::Begin("About",show, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("CIRCULATION");
        ImGui::Text("Cuda InteRactive Climate simULATION");
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

        ImGui::Separator();
        if(ImGui::Button("Close"))
            *show = false;
    }
    ImGui::End();
}

void Application::showKeybindingsWindow(bool* show)
{
    ImGui::SetNextWindowSize({0,0},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
                            ImGuiCond_Appearing, ImVec2(0.5f,0.5f));
    if(ImGui::Begin("Keybindings",show))
    {
        ImGui::Text("Keybindings on german keyboard:");

        if(ImGui::CollapsingHeader("General"))
        {
            ImGui::Columns(2);
            ImGui::Text("ESC"); ImGui::NextColumn(); ImGui::Text("Close Application"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("F11"); ImGui::NextColumn(); ImGui::Text("Toggle Fullscreen"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("TAB"); ImGui::NextColumn(); ImGui::Text("Toggle User Interface"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Columns(1);
        }

        if(ImGui::CollapsingHeader("Simulation"))
        {
            ImGui::Columns(2);
            ImGui::Text("1"); ImGui::NextColumn(); ImGui::Text("Resume Simulation"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("2"); ImGui::NextColumn(); ImGui::Text("Pause Simulation"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Columns(1);
        }

        if(ImGui::CollapsingHeader("Camera"))
        {
            ImGui::Columns(2);
            ImGui::Text("Left MB or CTRL + mouse"); ImGui::NextColumn(); ImGui::Text("Rotate"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("WASD"); ImGui::NextColumn(); ImGui::Text("Move"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("Q/E"); ImGui::NextColumn(); ImGui::Text("Move up / down"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("Middle MB or ALT + mouse"); ImGui::NextColumn(); ImGui::Text("Pan"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("Middle Wheel"); ImGui::NextColumn(); ImGui::Text("Zoom"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("hold SHIFT"); ImGui::NextColumn(); ImGui::Text("Slower movement"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("hold SPACE"); ImGui::NextColumn(); ImGui::Text("Faster movement"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("+/-"); ImGui::NextColumn(); ImGui::Text("increase / decrease movement speed"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("R"); ImGui::NextColumn(); ImGui::Text("switch between \"trackball\" and \"first person\" movement"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Text("X"); ImGui::NextColumn(); ImGui::Text("reset camera position and orientation"); ImGui::NextColumn(); ImGui::Separator();
            ImGui::Columns(1);
        }

        ImGui::Separator();
        if(ImGui::Button("Close"))
            *show = false;
    }
    ImGui::End();
}
