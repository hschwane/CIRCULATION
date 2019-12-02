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
    m_camera(mpu::gph::Camera::trackball, glm::vec3(0,0,2), glm::vec3(0,0,0)),
    m_renderer(width,height)
{
    // add shader include pathes
    mpu::gph::addShaderIncludePath(MPU_LIB_SHADER_PATH"include");
    mpu::gph::addShaderIncludePath(PROJECT_SHADER_PATH"include");

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
                                 this->m_renderer.setSize(w,h);
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
    if(m_showPerfWindow) showPerfWindow(&m_showPerfWindow);
    if(m_showAboutWindow) showAboutWindow(&m_showAboutWindow);
    if(m_showKeybindingsWindow) showKeybindingsWindow(&m_showKeybindingsWindow);
    if(m_showRendererWindow) m_renderer.showGui(&m_showRendererWindow);

    // open new simulation modal on startup
    static struct Once{Once(){ImGui::OpenPopup("New Simulation");}}once;
    newSimulationModal();

    // -------------------------
    // simulation

    // -------------------------
    // rendering
    m_camera.update();
    m_renderer.setViewMat(m_camera.viewMatrix());
    m_renderer.draw();

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
    glm::vec3 aabbMin{m_currentCS->getAABBMin().x,m_currentCS->getAABBMin().y, m_currentCS->getAABBMin().z};
    glm::vec3 aabbMax{m_currentCS->getAABBMax().x,m_currentCS->getAABBMax().y, m_currentCS->getAABBMax().z};

    glm::vec3 size = aabbMax - aabbMin;
    float diagonal = glm::length(size);
    glm::vec3 center = aabbMin + size/2;

    m_camera.setPosition(glm::vec3(diagonal));
    m_camera.setTarget(center);
}

void Application::mainMenuBar()
{
    bool newSimPressed=false; // was Simulation -> New selected?

    if(ImGui::BeginMainMenuBar())
    {
        // simulation menu to manage the simulation
        if(ImGui::BeginMenu("Simulation"))
        {
            if(ImGui::MenuItem("New"))
                newSimPressed=true; // needed for some imGui id stack thing
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Visualization"))
        {
            ImGui::MenuItem("Show Visualization window", nullptr, &m_showRendererWindow);
            ImGui::Separator();

            if(ImGui::MenuItem("Reset Camera","X"))
                resetCamera();

            if(ImGui::MenuItem("Toggle Camera Mode","R"))
                m_camera.toggleMode();

            ImGui::EndMenu();
        }

        // window menu to select shown windows
        if(ImGui::BeginMenu("Windows"))
        {
            ImGui::MenuItem("performance", nullptr, &m_showPerfWindow);
            ImGui::MenuItem("visualization", nullptr, &m_showRendererWindow);
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

        ImGui::EndMenuBar();
    }

    // open modal
    if(newSimPressed)
        ImGui::OpenPopup("New Simulation");
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
}

void Application::newSimulationModal()
{
    if(ImGui::BeginPopupModal("New Simulation",nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        static int selctedModel = 0;
        static int selctedCoordinates = 0;
        static int3 numGridCells{128,128,32};
        static float3 minCoords{-1,-1,-1};
        static float3 maxCoords{1,1,1};

        // select simulation model and coordinate system
        ImGui::Combo("Model",&selctedModel,"Render Demo\0\0");
        ImGui::Combo("Coordinate System",&selctedCoordinates,"2D Cartesian Coordinate\0\0");

        // figure out dimension
        auto cs = coordinateSystemFactory(static_cast<CSType>(selctedCoordinates),{0,0,0},{0,0,0},{0,0,0});

        // depending on the dimension number of selected system
        if(cs->getDimension() == 2)
        {
            ImGui::DragInt2("Number of Grid Cells", &numGridCells.x);
            ImGui::DragFloat2("Min coordinates", &minCoords.x);
            ImGui::DragFloat2("Max coordinates", &maxCoords.x);

            float3 size = maxCoords - minCoords;
            float3 cellSize = size / make_float3(numGridCells);
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::DragFloat2("Size", &size.x);
            ImGui::DragFloat2("Cell Size", &cellSize.x);
            int numOfCells = numGridCells.x * numGridCells.y;
            ImGui::DragInt("Total number of cells", &numOfCells);
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::DragInt3("Number of Grid Cells", &numGridCells.x);
            ImGui::DragFloat3("Min coordinates", &minCoords.x);
            ImGui::DragFloat3("Max coordinates", &maxCoords.x);

            float3 size = maxCoords - minCoords;
            float3 cellSize = size / make_float3(numGridCells);
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::DragFloat3("Size", &size.x);
            ImGui::DragFloat3("Cell Size", &cellSize.x);
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }

        if(ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::SameLine();

        if(ImGui::Button("Create"))
        {
            ImGui::CloseCurrentPopup();

            if(cs->getDimension() == 2)
            {
                minCoords.z=0;
                maxCoords.z=0;
                numGridCells.z=0;
            }

            createNewSim(static_cast<SimModel>(selctedModel), static_cast<CSType>(selctedCoordinates), minCoords, maxCoords, numGridCells);
        }
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
}

void Application::createNewSim(SimModel model, CSType coordinateSystem, const float3& min, const float3& max, const int3& cells)
{
    logINFO("Application") << "Creating new simulation with sim model " << int(model) << " coordinate system "
                           << int(coordinateSystem) << " coordinate range [" << min << "|" << max << "] and grid cell count " << cells;

    m_currentCS = coordinateSystemFactory(coordinateSystem, min, max, cells);
    m_renderer.setCS(m_currentCS);

    switch(model)
    {
        case SimModel::renderDemo:
        {
            RenderDemoGrid(m_currentCS->getNumGridCells());
            generateDemoData(m_demoGrid);
            m_demoGrid.addRenderBufferToVao(m_renderer.getVAO(), 0);
            m_demoGrid.bindRenderBuffer(0, GL_SHADER_STORAGE_BUFFER);
            break;
        }
    }

    resetCamera();
}

void Application::generateDemoData(RenderDemoGrid& grid)
{
    std::default_random_engine rng(mpu::getRanndomSeed());
    std::normal_distribution<float> dist(10,4);

    for(int i : mpu::Range<int>(grid.size()))
    {
        float density = fmax(0,dist(rng));
        float velX = fmax(0,dist(rng));
        float velY = fmax(0,dist(rng));

        grid.write<AT::density>(i,density);
        grid.write<AT::velocityX>(i,velX);
        grid.write<AT::velocityY>(i,velY);
    }

    grid.swapAndRender();
}
