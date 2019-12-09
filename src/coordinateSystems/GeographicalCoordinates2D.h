/*
 * CIRCULATION
 * GeographicalCoordinates2D.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the GeographicalCoordinates2D class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_GEOGRAPHICALCOORDINATES2D_H
#define CIRCULATION_GEOGRAPHICALCOORDINATES2D_H

// includes
//--------------------
#include "CoordinateSystem.h"
//--------------------

//-------------------------------------------------------------------
/**
 * class GeographicalCoordinates2D
 *
 * 2D geographical coordinates (one layer). First component is longitude 0<long<2pi, second is latitude. Grid cell access is row major.
 * No bounds checking is done!
 *
 */
class GeographicalCoordinates2D : public CoordinateSystem
{
public:
    GeographicalCoordinates2D(float minLat, float maxLat, int3 numGridCells, float radius); //!< smallest and biggest allowed latitude values (0<lat<pi), number of grid cells and radius (only used for conversion to cartesian coordinates)
    ~GeographicalCoordinates2D() override = default;

    // convert
    float3 getCartesian(const float3& coord) const override; //!< converts a coordinate into cartesian coordinates
    float3 getCoord(const float3& cartesian) const override; //!< converts cartesian coordinate into this coordinate system

    // for quantities stored at cell center
    float3 getCellCoordinate(int cellId) const override; //!< get the coordinates of a specific cell
    float3 getCellCoordinate3d(const int3& cellId3d) const override; //!< get the coordinates of the multi dimensional cell id
    int getCellId(const float3& coord) const override; //!< get the the cell id that belongs coordinates "coord"
    int3 getCellId3d(const float3& coord) const override; //!< get the multi dimensional cell id

    // adjacency
    int getRightNeighbor(int cellId) const override; //!< get neighbors for given cell along first positive axis
    int getLeftNeighbor(int cellId) const override; //!< get neighbors for given cell along first negative axis
    int getForwardNeighbor(int cellId) const override; //!< get neighbors for given cell along second positive axis
    int getBackwardNeighbor(int cellId) const override; //!< get neighbors for given cell along negative axis
    int getUpNeighbor(int cellId) const override; //!< get neighbors for given cell along third positive axis
    int getDownNeighbor(int cellId) const override; //!< get neighbors for given cell along third megative axis

    // boundaries
    float3 getMinCoord() const override; //!< get the lower bound for all dimensions
    float3 getMaxCoord() const override; //!< get the upper bound for all dimensions
    int getNumGridCells() const override; //!< total number of grid cells
    int3 getNumGridCells3d() const override; //!< number of grid cells in each dimension

    // dimensions
    float3 getCellSize() const override; //! get the size of the cell in target coordinates (uniform grid)
    int getDimension() const override; //!< get the number of dimensions
    int getCartesianDimension() const override; //!< get the number of dimensions in cartesian coordinates (eg surface of sphere dim=2 cartesian_dim = 3)

    // bounding box
    float3 getAABBMin() const override; //!< get the lower left  bounding box corner in cartesian coords
    float3 getAABBMax() const override; //!< get the upper right bounding box corner in cartesian coords

    // openGL support
    virtual std::string getShaderDefine() const; //!< returns name of a file to be included in a shader which defines above functions in glsl
    virtual void setShaderUniforms(mpu::gph::ShaderProgram& shader) const; //!< sets the necessary uniforms to a shader that included th shader file from "getShaderFileName()" function

private:
    const float m_radius; //!< radius of the sphere shell
    const float m_minLat; //!< smallest possible latitude
    const float m_maxLat; //!< biggest possible latitude
    const int2 m_numGridCells; //!< number of cells in each dimension
    const int m_totalNumGridCells; //!< total number of cells
    const float2 m_min; //!< smallest possible coordinate in both directions i.e. lower left corner of the grid
    const float2 m_max; //!< biggest possible coordinate in both directions i.e. upper right corner of the grid
    const float2 m_size; //!< size of the grid in both coordinate directions (m_max - m_min)
    const float2 m_cellSize; //!< size of one grid cell in geographical coordinates
};


#endif //CIRCULATION_GEOGRAPHICALCOORDINATES2D_H
