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
    setViewMat(glm::mat4(1.0f));

    m_gridlineShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.vert"});
    m_gridlineShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.geom"});
    m_gridlineShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.frag"});

    m_gridCenterShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.vert"});
    m_gridCenterShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.frag"});

    glEnable(GL_LINE_SMOOTH);

    m_aspect = float(w)/float(h);
    rebuildProjectionMat();

    // initial settings
    glClearColor( m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, 1.0f);
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
            if(ImGui::DragFloat("Scale",&m_scale,0.1f,0.0001f,1000.0f))
            {
                m_model = glm::scale(glm::mat4(1.0f),glm::vec3(m_scale));
                m_gridlineShader.uniformMat4("modelMat", m_model);
                m_gridCenterShader.uniformMat4("modelMat", m_model);
                setClip(0.001,m_unscaledFar*m_scale);
                updateMVP();
            }
        }

        if(ImGui::CollapsingHeader("Grid lines"))
        {
            ImGui::Checkbox("show grid lines",&m_renderGridlines);
            if(ImGui::ColorEdit3("Color##linecolor",glm::value_ptr(m_gridlineColor)))
                m_gridlineShader.uniform3f("constantColor", m_gridlineColor);
        }

//        if(ImGui::CollapsingHeader("Grid points"))
//        {
//            ImGui::Checkbox("show grid center points",&m_renderGridpoints);
//            if(ImGui::ColorEdit3("Color##pointcolor",glm::value_ptr(m_gridpointColor)))
//                m_gridCenterShader.uniform3f("constantColor", m_gridpointColor);

//        }

        ImGui::End();
    }
}

void Renderer::setCS(std::shared_ptr<CoordinateSystem> cs)
{
    m_cs = cs;

    // set near / far
    glm::vec3 aabbMin{m_cs->getAABBMin().x,m_cs->getAABBMin().y, m_cs->getAABBMin().z};
    glm::vec3 aabbMax{m_cs->getAABBMax().x,m_cs->getAABBMax().y, m_cs->getAABBMax().z};

    glm::vec3 size = aabbMax - aabbMin;
    float diagonal = glm::length(size);
    m_unscaledFar = diagonal * 4;
    setClip(0.001,m_unscaledFar*m_scale);

    // compile grid - line shader
    m_gridlineShader.clearDefinitions();
    m_gridlineShader.addDefinition(glsp::definition(m_cs->getShaderDefine()) );
    m_gridlineShader.rebuild();
    m_cs->setShaderUniforms(m_gridlineShader);
    m_gridlineShader.uniform3f("constantColor", m_gridlineColor);
    m_gridlineShader.uniformMat4("viewMat", m_view);
    m_gridlineShader.uniformMat4("projectionMat", m_projection);
    m_gridlineShader.uniformMat4("modelMat", m_model);

    // compile grid center shader
    m_gridCenterShader.clearDefinitions();
    m_gridCenterShader.addDefinition(glsp::definition("RENDER_GRID_CELL_POINTS"));
    m_gridCenterShader.addDefinition(glsp::definition(m_cs->getShaderDefine()) );
    m_gridCenterShader.rebuild();
    m_cs->setShaderUniforms(m_gridCenterShader);
    m_gridCenterShader.uniform3f("constantColor", m_gridpointColor);
    m_gridCenterShader.uniformMat4("viewMat", m_view);
    m_gridCenterShader.uniformMat4("projectionMat", m_projection);
    m_gridCenterShader.uniformMat4("modelMat", m_model);
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
    // don't render if there is nothing to render
    if(!m_cs)
        return;

    m_vao.bind();

    // visualize grid outlines
    if(m_renderGridlines)
    {
        m_gridlineShader.use();
        glDrawArrays(GL_POINTS, 0, m_cs->getNumGridCells());
    }

    // visualize grid centerpoints
    if(m_renderGridpoints)
    {
        m_gridCenterShader.use();
        glDrawArrays(GL_POINTS, 0, m_cs->getNumGridCells());
    }
}

void Renderer::setViewMat(const glm::mat4& view)
{
    m_view = view;
    m_gridlineShader.uniformMat4("viewMat", m_view);
    m_gridCenterShader.uniformMat4("viewMat", m_view);
    updateMVP();
}

void Renderer::rebuildProjectionMat()
{
    m_projection = glm::perspective(glm::radians(m_fovy),m_aspect,m_near,m_far);
    m_gridlineShader.uniformMat4("projectionMat", m_projection);
    m_gridCenterShader.uniformMat4("projectionMat", m_projection);
    updateMVP();
}

void Renderer::updateMVP()
{
    m_gridlineShader.uniformMat4("modelViewProjectionMat", m_projection * m_view * m_model);
    m_gridCenterShader.uniformMat4("modelViewProjectionMat", m_projection * m_view * m_model);
}