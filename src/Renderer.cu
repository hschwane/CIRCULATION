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

    m_renderShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.vert"});
    m_renderShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.geom"});
    m_renderShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.frag"});

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
        if(ImGui::ColorEdit3("Background",glm::value_ptr(m_backgroundColor)))
            glClearColor( m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, 1.0f);

        ImGui::End();
    }
}

void Renderer::setCS(std::shared_ptr<CoordinateSystem> cs)
{
    m_cs = cs;
    m_renderShader.clearDefinitions();
    m_renderShader.addDefinition(glsp::definition(m_cs->getShaderDefine()) );
    m_renderShader.rebuild();
    m_cs->setShaderUniforms(m_renderShader);
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

void Renderer::setViewMat(const glm::mat4& view)
{
    m_view = view;
    m_renderShader.uniformMat4("viewMat",m_view);
    m_renderShader.uniformMat4("viewProjectionMat",m_projection * m_view);
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
    m_renderShader.use();
    glDrawArrays(GL_POINTS,0,m_cs->getNumGridCells());
}

void Renderer::rebuildProjectionMat()
{
    m_projection = glm::perspective(glm::radians(m_fovy),m_aspect,m_near,m_far);
    m_renderShader.uniformMat4("projectionMat",m_projection);
    m_renderShader.uniformMat4("viewProjectionMat",m_projection * m_view);
}
