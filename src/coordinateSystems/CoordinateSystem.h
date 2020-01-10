/*
 * CIRCULATION
 * CoordinateSystem.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the CoordinateSystem class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_COORDINATESYSTEM_H
#define CIRCULATION_COORDINATESYSTEM_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpCuda.h>
#include <mpUtils/mpGraphics.h>

#include "../enums.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class CoordinateSystem
 *
 * usage:
 * Base class for all coordinate systems. Provides coordinate conversions to/from cartesian
 * coordinates and access to grid cells and face.
 * Also provides information about adjacent cells.
 * No direct support for staggered grid. As a convention each cell stores face/corner values belonging
 * to its upper right.
 *
 */
class CoordinateSystem
{
public:

    CUDAHOSTDEV virtual ~CoordinateSystem() = default;

    // convert
    CUDAHOSTDEV virtual float3 getCartesian(const float3& coord) const =0; //!< converts a coordinate into cartesian coordinates
    CUDAHOSTDEV virtual float3 getCoord(const float3& cartesian) const =0; //!< converts cartesian coordinate into this coordinate system

    // unit vectors
    CUDAHOSTDEV virtual float3 getUnitVectorX(float3 position) const =0; //!< get the unit vector of the first coordinate at position
    CUDAHOSTDEV virtual float3 getUnitVectorY(float3 position) const =0; //!< get the unit vector of the second coordinate at position
    CUDAHOSTDEV virtual float3 getUnitVectorZ(float3 position) const =0; //!< get the unit vector of the third coordinate at position

    // for quantities stored at cell center
    CUDAHOSTDEV virtual float3 getCellCoordinate(int cellId) const =0; //!< get the coordinates of a specific cell
    CUDAHOSTDEV virtual float3 getCellCoordinate3d(const int3& cellId3d) const =0; //!< get the coordinates of the multi dimensional cell id
    CUDAHOSTDEV virtual int getCellId(const float3& coord) const =0; //!< get the the cell id that belongs coordinates "coord"
    CUDAHOSTDEV virtual int getCellId(const int3& cellId3d) const =0; //!< get the the cell id from the multidimensional cell id
    CUDAHOSTDEV virtual int3 getCellId3d(const float3& coord) const =0; //!< get the multi dimensional cell id
    CUDAHOSTDEV virtual int3 getCellId3d(int cellId) const=0; //!< get the multi dimensional cell id from 1d cell id


    // adjacency
    CUDAHOSTDEV virtual int getRightNeighbor(int cellId) const =0; //!< get neighbors for given cell along first positive axis
    CUDAHOSTDEV virtual int getLeftNeighbor(int cellId) const =0; //!< get neighbors for given cell along first negative axis
    CUDAHOSTDEV virtual int getForwardNeighbor(int cellId) const =0; //!< get neighbors for given cell along second positive axis
    CUDAHOSTDEV virtual int getBackwardNeighbor(int cellId) const =0; //!< get neighbors for given cell along negative axis
    CUDAHOSTDEV virtual int getUpNeighbor(int cellId) const =0; //!< get neighbors for given cell along third positive axis
    CUDAHOSTDEV virtual int getDownNeighbor(int cellId) const =0; //!< get neighbors for given cell along third megative axis

    // boundaries
    CUDAHOSTDEV virtual float3 getMinCoord() const =0; //!< get the lower bound for all dimensions
    CUDAHOSTDEV virtual float3 getMaxCoord() const =0; //!< get the upper bound for all dimensions
    CUDAHOSTDEV virtual int getNumGridCells() const =0; //!< total number of grid cells
    CUDAHOSTDEV virtual int3 getNumGridCells3d() const =0; //!< number of grid cells in each dimension
    CUDAHOSTDEV virtual int3 hasBoundary() const =0; //!< 1 for each dimension which has a boundary, 0 if the dimension does not require a boundary (eg is periodic)

    // dimensions
    CUDAHOSTDEV virtual float3 getCellSize() const =0; //! get the size of the cell in target coordinates (uniform grid)
    CUDAHOSTDEV virtual int getDimension() const =0; //!< get the number of dimensions 1-3
    CUDAHOSTDEV virtual int getCartesianDimension() const =0; //!< get the number of dimensions in cartesian coordinates 1-3 (eg surface of sphere dim=2 cartesian_dim = 3)

    // bounding box
    CUDAHOSTDEV virtual float3 getAABBMin() const =0; //!< get the lower left  bounding box corner in cartesian coords
    CUDAHOSTDEV virtual float3 getAABBMax() const =0; //!< get the upper right bounding box corner in cartesian coords

    // openGL support
    virtual std::string getShaderDefine() const =0; //!< returns name of a file to be included in a shader which defines above functions in glsl
    virtual void setShaderUniforms(mpu::gph::ShaderProgram& shader) const =0; //!< sets the necessary uniforms to a shader that included th shader file from "getShaderFileName()" function

    // type for downcasting
    virtual CSType getType()=0; //!< identify the type of coordinate system using CSType from enums.h for downcasting
};


#endif //CIRCULATION_COORDINATESYSTEM_H
