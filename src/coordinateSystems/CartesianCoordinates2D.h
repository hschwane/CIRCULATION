/*
 * CIRCULATION
 * CartesianCoordinates2D.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the CartesianCoordinates2D class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_CARTESIANCOORDINATES2D_H
#define CIRCULATION_CARTESIANCOORDINATES2D_H

// includes
//--------------------
#include "CoordinateSystem.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class CartesianCoordinates2D
 *
 * 2D cartesian grid in the x-y-plane. Grid cell access is row major.
 * No bounds checking is done!
 *
 */
class CartesianCoordinates2D : public CoordinateSystem
{
public:
    CartesianCoordinates2D(float3 min, float3 max, int3 numGridCells); //!< smallest value, biggest value and number of grid cells in each dimension
    CUDAHOSTDEV ~CartesianCoordinates2D() override = default;

    // convert
    CUDAHOSTDEV float3 getCartesian(const float3& coord) const override; //!< converts a coordinate into cartesian coordinates
    CUDAHOSTDEV float3 getCoord(const float3& cartesian) const override; //!< converts cartesian coordinate into this coordinate system

    // unit vectors
    CUDAHOSTDEV float3 getUnitVectorX(float3 position) const override; //!< get the unit vector of the first coordinate at position
    CUDAHOSTDEV float3 getUnitVectorY(float3 position) const override; //!< get the unit vector of the second coordinate at position
    CUDAHOSTDEV float3 getUnitVectorZ(float3 position) const override; //!< get the unit vector of the third coordinate at position

    // coordinates and ids
    CUDAHOSTDEV float3 getCellCoordinate(int cellId) const override; //!< get the coordinates of a specific cell
    CUDAHOSTDEV float3 getCellCoordinate3d(const int3& cellId3d) const override; //!< get the coordinates of the multi dimensional cell id
    CUDAHOSTDEV int getCellId(const float3& coord) const override; //!< get the the cell id that belongs coordinates "coord"
    CUDAHOSTDEV int getCellId(const int3& cellId3d) const override; //!< get the the cell id from the multidimensional cell id
    CUDAHOSTDEV int3 getCellId3d(const float3& coord) const override; //!< get the multi dimensional cell id
    CUDAHOSTDEV int3 getCellId3d(int cellId) const override; //!< get the multi dimensional cell id from 1d cell id

    // adjacency
    CUDAHOSTDEV int getRightNeighbor(int cellId) const override; //!< get neighbors for given cell along first positive axis
    CUDAHOSTDEV int getLeftNeighbor(int cellId) const override; //!< get neighbors for given cell along first negative axis
    CUDAHOSTDEV int getForwardNeighbor(int cellId) const override; //!< get neighbors for given cell along second positive axis
    CUDAHOSTDEV int getBackwardNeighbor(int cellId) const override; //!< get neighbors for given cell along negative axis
    CUDAHOSTDEV int getUpNeighbor(int cellId) const override; //!< get neighbors for given cell along third positive axis
    CUDAHOSTDEV int getDownNeighbor(int cellId) const override; //!< get neighbors for given cell along third megative axis

    // boundaries
    CUDAHOSTDEV float3 getMinCoord() const override; //!< get the lower bound for all dimensions
    CUDAHOSTDEV float3 getMaxCoord() const override; //!< get the upper bound for all dimensions
    CUDAHOSTDEV int getNumGridCells() const override; //!< total number of grid cells
    CUDAHOSTDEV int3 getNumGridCells3d() const override; //!< number of grid cells in each dimension
    CUDAHOSTDEV int3 hasBoundary() const override; //!< 1 for each dimension which has a boundary, 0 if the dimension does not require a boundary (eg is periodic)

    // dimensions
    CUDAHOSTDEV float3 getCellSize() const override; //! get the size of the cell in target coordinates (uniform grid)
    CUDAHOSTDEV int getDimension() const override; //!< get the number of dimensions
    CUDAHOSTDEV int getCartesianDimension() const override; //!< get the number of dimensions in cartesian coordinates (eg surface of sphere dim=2 cartesian_dim = 3)

    // bounding box
    CUDAHOSTDEV float3 getAABBMin() const override; //!< get the lower left  bounding box corner in cartesian coords
    CUDAHOSTDEV float3 getAABBMax() const override; //!< get the upper right bounding box corner in cartesian coords

    // openGL support
    std::string getShaderDefine() const override ; //!< returns name of a file to be included in a shader which defines above functions in glsl
    void setShaderUniforms(mpu::gph::ShaderProgram& shader) const override; //!< sets the necessary uniforms to a shader that included th shader file from "getShaderFileName()" function

    // downcasting and template things
    CUDAHOSTDEV CSType getType() const override; //!< identify the type of coordinate system using CSType from enums.h for downcasting
    static constexpr bool isCartesian{true}; //!< is the coordinate system a cartesian coordinate system

private:
    const float2 m_min; //!< smallest possible coordinate
    const float2 m_max; //!< highest possible coordinate
    const int2 m_numGridCells; //!< number of cells in each dimension
    const int m_totalNumGridCells; //!< total number of cells
    const float2 m_size; //!< m_max - m_min
    const float2 m_cellSize; //!< size of one grid cell
};


#endif //CIRCULATION_CARTESIANCOORDINATES2D_H
