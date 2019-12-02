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
#include "coordinateSystems/CoordinateSystem.h"
//--------------------


//-------------------------------------------------------------------
/**
 * class Renderer
 *
 * Class to render a simulation grid.
 *
 * usage:
 * Set coordinates using setCS.
 * Bin the grids buffers to the vao using getVAO() also bind them to the same positions of SSBO binding point.
 * Declare the grid using declareGrid().
 * Set view and projection matrix, then call draw(). Call Size() in your framebuffer resize callback.
 * To change setting show the renderer ui with showUi().
 *
 */
class Renderer
{
public:

    Renderer(int w, int h); //!< create renderer with width and height of window

    void setCS(std::shared_ptr<CoordinateSystem> cs); //!< sets the coordinate system
    mpu::gph::VertexArray& getVAO(); //!< get a reference to the vao so buffers can be bound to it

    void setSize(int w, int h); //!< call when window is resized
    void setClip(float near, float far); //!< change clipping distance
    void setViewMat(const glm::mat4& view); //!< set the view matrix

    void showGui(bool* show); //!< show user interface for rendering settings
    void draw(); //!< draw the grid

private:

    // settings
    glm::vec3 m_backgroundColor{0.2,0.2,0.2}; //!< background color
    bool m_renderGridlines{true};   //!< should grid lines be rendered
    glm::vec3 m_gridlineColor{1.0,1.0,1.0}; //!< gridline color
    bool m_renderGridpoints{false}; //!< should grid center points be rendered
    glm::vec3 m_gridpointColor{1.0,1.0,1.0}; //!< gridpoint color


    float m_near{0.001}; //!< near plane distance
    float m_far{50}; //!< far plane distance
    float m_fovy{60}; //!< field of view in degrees
    float m_aspect; //!< aspect ratio of the current window

    glm::mat4 m_projection{1.0}; //!< projection matrix used when rendering
    glm::mat4 m_view{1.0}; //!< view matrix used when rendering

    // stuff to render
    std::shared_ptr<CoordinateSystem> m_cs{nullptr}; //!< coordinate system to use for rendering

    // opengl objects
    mpu::gph::ShaderProgram m_gridlineShader; //!< shader used for rendering
    mpu::gph::ShaderProgram m_gridCenterShader; //!< shader used for rendering
    mpu::gph::VertexArray m_vao; //!< vertex array to use for rendering

    void rebuildProjectionMat(); //!< set the projection matrix using a aspect ratio

};


#endif //CIRCULATION_RENDERER_H
