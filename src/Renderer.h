/*
 * CIRCULATION
 * Renderer.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the Renderer class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_RENDERER_H
#define CIRCULATION_RENDERER_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpGraphics.h>
//--------------------

//-------------------------------------------------------------------
/**
 * class Renderer
 *
 * Class to render a simulation grid.
 *
 */
class Renderer
{
public:

    Renderer(int w, int h); //!< create renderer with width and height of window

    mpu::gph::VertexArray& getVAO(); //!< get a reference to the vao so buffers can be bound to it

    void setSize(int w, int h); //!< call when window is resized
    void setViewMat(const glm::mat4& view); //!< set the view matrix

    void showGui(bool* show); //!< show user interface for rendering settings
    void draw(); //!< draw the grid

private:

    // settings
    glm::vec3 m_backgroundColor{0.2,0.2,0.2}; //!< background color
    float m_scale{1.0}; //!< global scale factor
    bool m_backfaceCulling{false}; //!< is backface culling on / off?

    float m_near{0.001}; //!< near plane distance
    float m_far{50}; //!< far plane distance
    float m_unscaledFar{50}; //!< far plane without scaling
    float m_fovy{60}; //!< field of view in degrees
    float m_aspect; //!< aspect ratio of the current window

    bool m_renderIcosphere; //!< should the icosphere be rendered

    glm::mat4 m_projection{1.0}; //!< projection matrix used when rendering
    glm::mat4 m_view{1.0}; //!< view matrix used when rendering
    glm::mat4 m_model{1.0}; //!< model matrix used when rendering

    // opengl objects
    mpu::gph::ShaderProgram m_icosphereShader; //!< shader to draw the icosphere
    mpu::gph::VertexArray m_vao; //!< vertex array to use for rendering

    // internal helper functions
    void compileShader(); //!< comile / recompile all visualization shader
    void setClip(float near, float far); //!< change clipping distance
    void rebuildProjectionMat(); //!< set the projection matrix using a aspect ratio
    void setModelMat(const glm::mat4& m); //!< set the model matrix
    void updateMVP(); //!< update model  view projection on all shaders
    void setBackfaceCulling(bool enable); //!< enable / disable backface culling
};


#endif //CIRCULATION_RENDERER_H
