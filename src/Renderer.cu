/*
 * CIRCULATION
 * Renderer.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the Renderer class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "Renderer.h"
//--------------------

// function definitions of the Renderer class
//-------------------------------------------------------------------

Renderer::Renderer(int w, int h)
{
    // add shader include pathes
    mpu::gph::addShaderIncludePath(MPU_LIB_SHADER_PATH"include");
    mpu::gph::addShaderIncludePath(PROJECT_SHADER_PATH"include");

    // add shader files
    m_icosphereShader.setShaderModule({PROJECT_SHADER_PATH"icosahedron.vert"});
    m_icosphereShader.setShaderModule({PROJECT_SHADER_PATH"icosahedron.frag"});

    // initial values for matrices
    m_aspect = float(w)/float(h);
    m_view = glm::mat4(1.0f);
    m_model = glm::mat4(1.0f);
    // projection mat is build when compiling shaders

    // try compiling shaders
    compileShader();

    // initial settings
    glEnable(GL_DEPTH_TEST);
    glClearColor( m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, 1.0f);
    glPointSize(5.0f);
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
}

void Renderer::showGui(bool* show)
{
    ImGui::SetNextWindowSize({0,0},ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Visualization",show))
    {
        if(ImGui::CollapsingHeader("General"))
        {
            if(ImGui::ColorEdit3("Background",glm::value_ptr(m_backgroundColor)))
                glClearColor( m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, 1.0f);
            if(ImGui::DragFloat("Scale",&m_scale,0.01f,0.0001f,1000.0f))
            {
                setModelMat(glm::scale(glm::mat4(1.0f),glm::vec3(m_scale)));
                setClip(0.001,m_unscaledFar*m_scale);
            }

            if(ImGui::Checkbox("Hide back-faces",&m_backfaceCulling))
                setBackfaceCulling(m_backfaceCulling);

            if(ImGui::Button("Rebuild Shader"))
                compileShader();
        }
    }
    ImGui::End();
}

void Renderer::compileShader()
{
    logINFO("Renderer") << "recompiling all visualization shaders";

    try
    {
        m_icosphereShader.rebuild();
        setModelMat(m_model);
        setViewMat(m_view);
        rebuildProjectionMat();
        updateMVP();
    }
    catch (const std::runtime_error& e)
    {
        logERROR("Renderer") << "Shader compilation failed! Fix shader and try again.";
        int r = tinyfd_messageBox("Error","Shader compilation failed! Check shader and press \"ok\". \nSee log for more information.",
                "okcancel", "error",1);
        if(r == 0)
            throw e;
        else
            compileShader();
    }
}

void Renderer::setSize(int w, int h)
{
    m_aspect = float(w) / float(h);
    rebuildProjectionMat();
}

void Renderer::setClip(float near, float far)
{
    m_near = near;
    m_far = far;
    rebuildProjectionMat();
}

mpu::gph::VertexArray& Renderer::getVAO()
{
    return m_vao;
}

void Renderer::draw()
{
    m_vao.bind();

    // visualize scalar field
    if(m_renderIcosphere)
    {
        m_icosphereShader.use();
        glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);
    }
}

void Renderer::setNumIndices(int numGridpoints)
{
    m_numIndices = numGridpoints;
}

mpu::gph::VertexArray& Renderer::getVao()
{
    return m_vao;
}

void Renderer::setViewMat(const glm::mat4& view)
{
    m_view = view;
    m_icosphereShader.uniformMat4("viewMat", m_view);
    updateMVP();
}

void Renderer::rebuildProjectionMat()
{
    m_projection = glm::perspective(glm::radians(m_fovy),m_aspect,m_near,m_far);
    m_icosphereShader.uniformMat4("projectionMat", m_projection);
    updateMVP();
}

void Renderer::setModelMat(const glm::mat4& m)
{
    m_model = m;
    m_icosphereShader.uniformMat4("modelMat", m_model);
    updateMVP();
}

void Renderer::updateMVP()
{
    m_icosphereShader.uniformMat4("modelViewProjectionMat", m_projection * m_view * m_model);
}

void Renderer::setBackfaceCulling(bool enable)
{
    m_backfaceCulling = enable;
    if(enable)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

}