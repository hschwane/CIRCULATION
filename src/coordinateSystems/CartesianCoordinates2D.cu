/*
 * CIRCULATION
 * CartesianCoordinates2D.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the CartesianCoordinates2D class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "CartesianCoordinates2D.h"
//--------------------

// namespace
//--------------------

//--------------------

// function definitions of the CartesianCoordinates2D class
//-------------------------------------------------------------------
CartesianCoordinates2D::CartesianCoordinates2D(float3 min, float3 max, int3 numGridCells)
    : m_min(make_float2(min)), m_max(make_float2(max)), m_numGridCells(make_int2(numGridCells)),
    m_totalNumGridCells(numGridCells.x*numGridCells.y),
    m_size(m_max-m_min),
    m_cellSize( m_size / make_float2(m_numGridCells))
{
}

float3 CartesianCoordinates2D::getCartesian(const float3& coord) const
{
    return float3{coord.x,coord.y,0};
}

float3 CartesianCoordinates2D::getCoord(const float3& cartesian) const
{
    return float3{cartesian.x,cartesian.y,0};
}

float3 CartesianCoordinates2D::getCellCoordinate(int cellId) const
{
    return getCellCoordinate3d({cellId%m_numGridCells.x, cellId/m_numGridCells.x,0});
}

float3 CartesianCoordinates2D::getCellCoordinate3d(const int3& cellId3d) const
{
    int2 cellId2d = make_int2(cellId3d);
    float2 coord2d = make_float2(cellId2d) * m_cellSize + m_min;
    return make_float3(coord2d);
}

int CartesianCoordinates2D::getCellId(const float3& coord) const
{
    int3 cellId3d = getCellId3d(coord);
    return cellId3d.y*m_numGridCells.x+ cellId3d.x;
}

int3 CartesianCoordinates2D::getCellId3d(const float3& coord) const
{
    float2 coord2d =  (make_float2(coord) - m_min) / m_cellSize;
    return make_int3(rintf(coord2d.x),rintf(coord2d.y),0);
}

int CartesianCoordinates2D::getRightNeighbor(int cellId) const
{
    return cellId+1;
}

int CartesianCoordinates2D::getLeftNeighbor(int cellId) const
{
    return cellId-1;
}

int CartesianCoordinates2D::getForwardNeighbor(int cellId) const
{
    return cellId+m_numGridCells.x;
}

int CartesianCoordinates2D::getBackwardNeighbor(int cellId) const
{
    return cellId-m_numGridCells.x;
}

int CartesianCoordinates2D::getUpNeighbor(int cellId) const
{
    return -1;
}

int CartesianCoordinates2D::getDownNeighbor(int cellId) const
{
    return -1;
}

int CartesianCoordinates2D::numGridCells() const
{
    return m_totalNumGridCells;
}

int3 CartesianCoordinates2D::numGridCells3d() const
{
    return make_int3(m_numGridCells);
}

float3 CartesianCoordinates2D::getCellSize() const
{
    return make_float3(m_cellSize);
}

float3 CartesianCoordinates2D::getMinCoord() const
{
    return float3{m_min.x, m_min.y, 0};
}

float3 CartesianCoordinates2D::getMaxCoord() const
{
    return float3{m_max.x,m_max.y,0};
}

int CartesianCoordinates2D::getDimension() const
{
    return 2;
}

int CartesianCoordinates2D::getCartesianDimension() const
{
    return 2;
}

float3 CartesianCoordinates2D::getAABBMin() const
{
    return float3{m_min.x, m_min.y, 0};
}

float3 CartesianCoordinates2D::getAABBMax() const
{
    return float3{m_max.x,m_max.y,0};
}
