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

    virtual ~CoordinateSystem() = default;

    // convert
    virtual float3 getCartesian(const float3& coord) const =0; //!< converts a coordinate into cartesian coordinates
    virtual float3 getCoord(const float3& cartesian) const =0; //!< converts cartesian coordinate into this coordinate system

    // for quantities stored at cell center
    virtual float3 getCellCoordinate(int cellId) const =0; //!< get the coordinates of a specific cell
    virtual float3 getCellCoordinate3d(const int3& cellId3d) const =0; //!< get the coordinates of the multi dimensional cell id
    virtual int getCellId(const float3& coord) const =0; //!< get the the cell id that belongs coordinates "coord"
    virtual int3 getCellId3d(const float3& coord) const =0; //!< get the multi dimensional cell id

    // adjacency
    virtual int getRightNeighbor(int cellId) const =0; //!< get neighbors for given cell along first positive axis
    virtual int getLeftNeighbor(int cellId) const =0; //!< get neighbors for given cell along first negative axis
    virtual int getForwardNeighbor(int cellId) const =0; //!< get neighbors for given cell along second positive axis
    virtual int getBackwardNeighbor(int cellId) const =0; //!< get neighbors for given cell along negative axis
    virtual int getUpNeighbor(int cellId) const =0; //!< get neighbors for given cell along third positive axis
    virtual int getDownNeighbor(int cellId) const =0; //!< get neighbors for given cell along third megative axis

    // boundaries
    virtual float3 getMinCoord() const =0; //!< get the lower bound for all dimensions
    virtual float3 getMaxCoord() const =0; //!< get the upper bound for all dimensions
    virtual int getNumGridCells() const =0; //!< total number of grid cells
    virtual int3 getNumGridCells3d() const =0; //!< number of grid cells in each dimension

    // dimensions
    virtual float3 getCellSize() const =0; //! get the size of the cell in target coordinates (uniform grid)
    virtual int getDimension() const =0; //!< get the number of dimensions 1-3
    virtual int getCartesianDimension() const =0; //!< get the number of dimensions in cartesian coordinates 1-3 (eg surface of sphere dim=2 cartesian_dim = 3)

    // bounding box
    virtual float3 getAABBMin() const =0; //!< get the lower left  bounding box corner in cartesian coords
    virtual float3 getAABBMax() const =0; //!< get the upper right bounding box corner in cartesian coords

    // openGL support
    virtual std::string getShaderDefine() const =0; //!< returns name of a file to be included in a shader which defines above functions in glsl
    virtual void setShaderUniforms(mpu::gph::ShaderProgram& shader) const =0; //!< sets the necessary uniforms to a shader that included th shader file from "getShaderFileName()" function
};


#endif //CIRCULATION_COORDINATESYSTEM_H
