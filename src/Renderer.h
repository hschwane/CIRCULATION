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

    void setScalarFields(std::vector<std::pair<std::string,int>> fields); //!< set scalar fields names and buffer ids
    void setVecFields(std::vector<std::pair<std::string,std::pair<int,int>>> fields); //!< set vextor fields names and buffer ids
    void setSize(int w, int h); //!< call when window is resized
    void setViewMat(const glm::mat4& view); //!< set the view matrix

    void showGui(bool* show); //!< show user interface for rendering settings
    void draw(); //!< draw the grid

private:

    // settings
    glm::vec3 m_backgroundColor{0.2,0.2,0.2}; //!< background color
    float m_scale{1.0}; //!< global scale factor
    bool m_backfaceCulling{false}; //!< is backface culling on / off?
    bool m_colorCodeCellID{false}; //!< is backface culling on / off?

    bool m_renderGridlines{false};   //!< should grid lines be rendered
    glm::vec3 m_gridlineColor{1.0,1.0,1.0}; //!< gridline color

    bool m_renderGridpoints{false}; //!< should grid center points be rendered
    glm::vec3 m_gridpointColor{1.0,1.0,1.0}; //!< gridpoint color

    bool m_renderScalarField{true}; //!< should a scalar field be rendered
    glm::vec3 m_scalarConstColor{0.8,0.8,0.8}; //!< gridpoint color
    glm::vec3 m_scalarMinColor{0.0,0.0,0.0}; //!< color of smallest value
    glm::vec3 m_scalarMaxColor{1.0,0.0,0.0}; //!< color of biggest value
    float m_minScalar{0.0f}; //!< smallest scalar value
    float m_maxScalar{1.0f}; //!< biggest scalar value
    float m_gap{0.0f}; //!< size of gap between scalar values
    int m_currentScalarField{-1}; //!< scalar field to visualize

    bool m_renderVectorField{true}; //!< should a vector field be rendered
    float m_arrowSize{0.013}; //!< size of the drawn arrow sprites
    glm::vec3 m_VectorConstColor{0.0,0.8,1.0}; //!< vector color
    bool m_colorVectorsByLength{false}; //!< should vectors be colored by length, or constant?
    glm::vec3 m_minVecColor{0.0,0.0,0.0}; //!< color of smallest value
    glm::vec3 m_maxVecColor{0.0,0.0,1.0}; //!< color of biggest value
    float m_minVecLength{0.0f}; //!< smallest scalar value
    float m_maxVecLength{1.0f}; //!< biggest scalar value
    int m_currentVecField{-1};

    float m_near{0.001}; //!< near plane distance
    float m_far{50}; //!< far plane distance
    float m_unscaledFar{50}; //!< far plane without scaling
    float m_fovy{60}; //!< field of view in degrees
    float m_aspect; //!< aspect ratio of the current window

    glm::mat4 m_projection{1.0}; //!< projection matrix used when rendering
    glm::mat4 m_view{1.0}; //!< view matrix used when rendering
    glm::mat4 m_model{1.0}; //!< model matrix used when rendering

    // stuff to render
    std::shared_ptr<CoordinateSystem> m_cs{nullptr}; //!< coordinate system to use for rendering
    std::vector<std::pair<std::string,int>> m_scalarFields; //!< scalar fields used for visualization, name and buffer id
    std::vector<std::pair<std::string,std::pair<int,int>>> m_vectorFields; //!< scalar fields used for visualization, name and buffer id

    // opengl objects
    mpu::gph::ShaderProgram m_vectorShader; //!< shader used for rendering
    mpu::gph::ShaderProgram m_scalarShader; //!< shader used for rendering
    mpu::gph::ShaderProgram m_gridlineShader; //!< shader used for rendering
    mpu::gph::ShaderProgram m_gridCenterShader; //!< shader used for rendering
    mpu::gph::VertexArray m_vao; //!< vertex array to use for rendering

    // internal helper functions
    void compileShader(); //!< comile / recompile all visualization shader
    void setClip(float near, float far); //!< change clipping distance
    void rebuildProjectionMat(); //!< set the projection matrix using a aspect ratio
    void updateMVP(); //!< update model  view projection on all shaders
    void setBackfaceCulling(bool enable); //!< enable / disable backface culling
};


#endif //CIRCULATION_RENDERER_H
