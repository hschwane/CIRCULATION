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
    m_gridlineShader.setShaderModule({PROJECT_SHADER_PATH"gridineRenderer.geom"});
    m_gridlineShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.frag"});

    m_gridCenterShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.vert"});
    m_gridCenterShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.frag"});

    m_scalarShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.vert"});
    m_scalarShader.setShaderModule({PROJECT_SHADER_PATH"scalarRenderer.geom"});
    m_scalarShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.frag"});

    m_vectorShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.vert"});
    m_vectorShader.setShaderModule({PROJECT_SHADER_PATH"vectorRenderer.geom"});
    m_vectorShader.setShaderModule({PROJECT_SHADER_PATH"gridRenderer.frag"});

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
            if(ImGui::DragFloat("Scale",&m_scale,0.01f,0.0001f,1000.0f))
            {
                m_model = glm::scale(glm::mat4(1.0f),glm::vec3(m_scale));
                m_gridlineShader.uniformMat4("modelMat", m_model);
                m_gridCenterShader.uniformMat4("modelMat", m_model);
                setClip(0.001,m_unscaledFar*m_scale);
                updateMVP();
            }
            if(ImGui::Button("Rebuild Shader"))
                compileShader();
        }

        if(ImGui::CollapsingHeader("Scalar field"))
        {
            ImGui::Checkbox("show scalar field",&m_renderScalarField);
            if(ImGui::DragFloat("gap between cells", &m_gap, 0.0001f,0.0000000001f,20.0f,"%.4f"))
                m_scalarShader.uniform1f("gapSize",m_gap);

            if( ImGui::BeginCombo("Attribute##scalarfieldselection", (m_currentScalarField<0) ? "Solid Color"
                                            : m_scalarFields[m_currentScalarField].first.c_str() ))
            {
                bool selected = (m_currentScalarField == -1);
                if(ImGui::Selectable("Solid Color##scalarfieldselection", &selected))
                {
                    m_currentScalarField = -1;
                    m_scalarShader.uniform1b("scalarColor",false);
                }

                for(int i = 0; i < m_scalarFields.size(); i++)
                {
                    selected = (m_currentScalarField == i);
                    if(ImGui::Selectable((m_scalarFields[i].first + "##scalarfieldselection").c_str(), &selected))
                    {
                        m_currentScalarField = i;
                        m_scalarShader.uniform1b("scalarColor",true);
                        glBindAttribLocation(static_cast<GLuint>(m_scalarShader), m_scalarFields[i].second, "scalar");
                    }
                }
                ImGui::EndCombo();
            }

            if(m_currentScalarField == -1)
            {
                if(ImGui::ColorEdit3("Color##scalarconstcolor", glm::value_ptr(m_scalarConstColor)))
                    m_scalarShader.uniform3f("constantColor", m_scalarConstColor);
            } else
            {
                if(ImGui::ColorEdit3("Min Color##scalarmnincolor", glm::value_ptr(m_scalarMinColor)))
                    m_scalarShader.uniform3f("minScalarColor", m_scalarMinColor);
                if(ImGui::ColorEdit3("Max Color##scalarmaxcolor", glm::value_ptr(m_scalarMaxColor)))
                    m_scalarShader.uniform3f("maxScalarColor", m_scalarMaxColor);
                if(ImGui::DragFloat("Min Value##minscalarcolor",&m_minScalar,0.01))
                    m_scalarShader.uniform1f("minScalar",m_minScalar);
                if(ImGui::DragFloat("Max Value##maxscalarcolor",&m_maxScalar,0.01))
                    m_scalarShader.uniform1f("maxScalar",m_maxScalar);
            }
        }

        if(ImGui::CollapsingHeader("Vector field"))
        {
            ImGui::Text("Note: arrows show the direction of the vector.");

            ImGui::Checkbox("show vector field",&m_renderVectorField);

            if( ImGui::BeginCombo("Attribute##vectorfieldselection", (m_currentVecField<0) ? "none" : m_vectorFields[m_currentVecField].first.c_str()))
            {
                for(int i = 0; i < m_vectorFields.size(); i++)
                {
                    bool selected = (m_currentVecField == i);
                    if(ImGui::Selectable((m_vectorFields[i].first+"##vectorfieldselection").c_str(), &selected))
                    {
                        m_currentVecField = i;

                        unsigned int blockIndex = 0;
                        blockIndex = glGetProgramResourceIndex(static_cast<GLuint>(m_vectorShader), GL_SHADER_STORAGE_BLOCK, "vectorFieldX");
                        glShaderStorageBlockBinding(static_cast<GLuint>(m_vectorShader), blockIndex, m_vectorFields[i].second.first);

                        blockIndex = glGetProgramResourceIndex(static_cast<GLuint>(m_vectorShader), GL_SHADER_STORAGE_BLOCK, "vectorFieldY");
                        glShaderStorageBlockBinding(static_cast<GLuint>(m_vectorShader), blockIndex, m_vectorFields[i].second.second);
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Checkbox("color vectors by length##vectorfieldselection",&m_colorVectorsByLength);
                m_vectorShader.uniform1b("scalarColor",m_colorVectorsByLength);

            if(m_colorVectorsByLength)
            {
                if(ImGui::ColorEdit3("Min Color##vecmincolor", glm::value_ptr(m_minVecColor)))
                    m_vectorShader.uniform3f("minScalarColor", m_minVecColor);
                if(ImGui::ColorEdit3("Max Color##vecmaxcolor", glm::value_ptr(m_maxVecColor)))
                    m_vectorShader.uniform3f("maxScalarColor", m_maxVecColor);
                if(ImGui::DragFloat("Min Length##minveclength",&m_minVecLength,0.01))
                    m_vectorShader.uniform1f("minScalar",m_minVecLength);
                if(ImGui::DragFloat("Max Length##maxveclength",&m_maxVecLength,0.01))
                    m_vectorShader.uniform1f("maxScalar",m_maxVecLength);
            } else
            {
                if(ImGui::ColorEdit3("Color##vecconstcolor", glm::value_ptr(m_VectorConstColor)))
                    m_vectorShader.uniform3f("constantColor", m_VectorConstColor);
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

    compileShader();
}

void Renderer::compileShader()
{
    logINFO("Renderer") << "recompiling all visualization shdaders";

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

    // compile scalar field shader
    m_scalarShader.clearDefinitions();
    m_scalarShader.addDefinition(glsp::definition(m_cs->getShaderDefine()) );
    m_scalarShader.rebuild();
    m_cs->setShaderUniforms(m_scalarShader);
    m_scalarShader.uniform3f("constantColor", m_scalarConstColor);
    m_scalarShader.uniformMat4("viewMat", m_view);
    m_scalarShader.uniformMat4("projectionMat", m_projection);
    m_scalarShader.uniformMat4("modelMat", m_model);
    m_scalarShader.uniform1f("gapSize", m_gap);
    m_scalarShader.uniform3f("minScalarColor", m_scalarMinColor);
    m_scalarShader.uniform3f("maxScalarColor", m_scalarMaxColor);
    m_scalarShader.uniform1f("minScalar",m_minScalar);
    m_scalarShader.uniform1f("maxScalar",m_maxScalar);
    m_scalarShader.uniform1b("scalarColor",(m_currentScalarField >= 0));
    glBindAttribLocation(static_cast<GLuint>(m_scalarShader),
                         (m_currentScalarField >= 0) ? m_scalarFields[m_currentScalarField].second : 0, "scalar");

    // compile vector shader
    m_vectorShader.clearDefinitions();
    m_vectorShader.addDefinition(glsp::definition(m_cs->getShaderDefine()) );
    m_vectorShader.rebuild();
    m_cs->setShaderUniforms(m_vectorShader);
    m_vectorShader.uniformMat4("viewMat", m_view);
    m_vectorShader.uniformMat4("projectionMat", m_projection);
    m_vectorShader.uniformMat4("modelMat", m_model);
    m_vectorShader.uniform1b("prepareVector", true);
    m_vectorShader.uniform1b("scalarColor",m_colorVectorsByLength);
    m_vectorShader.uniform3f("minScalarColor", m_minVecColor);
    m_vectorShader.uniform3f("maxScalarColor", m_maxVecColor);
    m_vectorShader.uniform1f("minScalar",m_minVecLength);
    m_vectorShader.uniform1f("maxScalar",m_maxVecLength);
    m_vectorShader.uniform3f("constantColor", m_VectorConstColor);

    if(m_currentVecField >= 0)
    {
        unsigned int blockIndex = 0;
        blockIndex = glGetProgramResourceIndex(static_cast<GLuint>(m_vectorShader), GL_SHADER_STORAGE_BLOCK,"vectorFieldX");
        glShaderStorageBlockBinding(static_cast<GLuint>(m_vectorShader), blockIndex,m_vectorFields[m_currentVecField].second.first);

        blockIndex = glGetProgramResourceIndex(static_cast<GLuint>(m_vectorShader), GL_SHADER_STORAGE_BLOCK,"vectorFieldY");
        glShaderStorageBlockBinding(static_cast<GLuint>(m_vectorShader), blockIndex,m_vectorFields[m_currentVecField].second.second);
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
    // don't render if there is nothing to render
    if(!m_cs)
        return;

    m_vao.bind();

    // visualize scalar field
    if(m_renderScalarField)
    {
        m_scalarShader.use();
        glDrawArrays(GL_POINTS, 0, m_cs->getNumGridCells());
    }

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

    // visualize vectors
    if(m_renderVectorField && m_currentVecField >= 0)
    {
        m_vectorShader.use();
        glDrawArrays(GL_POINTS, 0, m_cs->getNumGridCells());
    }
}

void Renderer::setViewMat(const glm::mat4& view)
{
    m_view = view;
    m_gridlineShader.uniformMat4("viewMat", m_view);
    m_gridCenterShader.uniformMat4("viewMat", m_view);
    m_scalarShader.uniformMat4("viewMat", m_view);
    m_vectorShader.uniformMat4("viewMat", m_view);
    updateMVP();
}

void Renderer::rebuildProjectionMat()
{
    m_projection = glm::perspective(glm::radians(m_fovy),m_aspect,m_near,m_far);
    m_gridlineShader.uniformMat4("projectionMat", m_projection);
    m_gridCenterShader.uniformMat4("projectionMat", m_projection);
    m_scalarShader.uniformMat4("projectionMat", m_projection);
    m_vectorShader.uniformMat4("projectionMat", m_projection);
    updateMVP();
}

void Renderer::updateMVP()
{
    m_gridlineShader.uniformMat4("modelViewProjectionMat", m_projection * m_view * m_model);
    m_gridCenterShader.uniformMat4("modelViewProjectionMat", m_projection * m_view * m_model);
    m_scalarShader.uniformMat4("modelViewProjectionMat", m_projection * m_view * m_model);
    m_vectorShader.uniformMat4("modelViewProjectionMat", m_projection * m_view * m_model);
}

void Renderer::setScalarFields(std::vector<std::pair<std::string, int>> fields)
{
    m_currentScalarField = -1;
    m_scalarFields = std::move(fields);
}

void Renderer::setVecFields(std::vector<std::pair<std::string,std::pair<int,int>>> fields)
{
    m_currentVecField = -1;
    m_vectorFields = std::move(fields);
}
