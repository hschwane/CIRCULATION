/*
 * CIRCULATION
 * GeographicalCoordinates2D.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the GeographicalCoordinates2D class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "GeographicalCoordinates2D.h"
//--------------------

// function definitions of the GeographicalCoordinates2D class
//-------------------------------------------------------------------

GeographicalCoordinates2D::GeographicalCoordinates2D(float minLat, float maxLat, int3 numGridCells, float radius)
    : m_radius(radius), m_numGridCells(make_int2(numGridCells)),
        m_min(make_float2(0,minLat)), m_max(make_float2(2* M_PIf32, maxLat)),
        m_totalNumGridCells(numGridCells.x*numGridCells.y), m_size(m_max - m_min),
        m_cellSize( m_size / make_float2(m_numGridCells))
{
}

float3 GeographicalCoordinates2D::getCartesian(const float3& coord) const
{
    float phi = M_PI_2f32 - coord.y;
    float sinPhi = sin(phi);
    return make_float3( m_radius * cos(coord.x) * sinPhi, m_radius * sin(coord.x) * sinPhi, m_radius * cos(phi));
}

float3 GeographicalCoordinates2D::getCoord(const float3& cartesian) const
{
    float r = sqrt( cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z );
    float phi = acos(cartesian.z / r);
    return make_float3(  atan2(cartesian.y,cartesian.x), M_PI_2f32 - phi, 0);
}

float3 GeographicalCoordinates2D::getCellCoordinate(int cellId) const
{
    return getCellCoordinate3d({cellId%m_numGridCells.x, cellId/m_numGridCells.x,0});
}

float3 GeographicalCoordinates2D::getCellCoordinate3d(const int3& cellId3d) const
{
    int2 cellId2d = make_int2(cellId3d);
    float2 coord2d = make_float2(cellId2d) * m_cellSize + m_min;
    return make_float3(coord2d);
}

int GeographicalCoordinates2D::getCellId(const float3& coord) const
{
    int3 cellId3d = getCellId3d(coord);
    return cellId3d.y*m_numGridCells.x+ cellId3d.x;
}

int3 GeographicalCoordinates2D::getCellId3d(const float3& coord) const
{
    float2 coord2d =  (make_float2(coord) - m_min) / m_cellSize;
    return make_int3(rintf(coord2d.x),rintf(coord2d.y),0);
}

int GeographicalCoordinates2D::getRightNeighbor(int cellId) const
{
    return cellId+1;
}

int GeographicalCoordinates2D::getLeftNeighbor(int cellId) const
{
    return cellId-1;
}

int GeographicalCoordinates2D::getForwardNeighbor(int cellId) const
{
    return cellId+m_numGridCells.x;
}

int GeographicalCoordinates2D::getBackwardNeighbor(int cellId) const
{
    return cellId-m_numGridCells.x;
}

int GeographicalCoordinates2D::getUpNeighbor(int cellId) const
{
    return -1;
}

int GeographicalCoordinates2D::getDownNeighbor(int cellId) const
{
    return -1;
}

float3 GeographicalCoordinates2D::getMinCoord() const
{
    return make_float3(m_min);
}

float3 GeographicalCoordinates2D::getMaxCoord() const
{
    return make_float3(m_max);
}

int GeographicalCoordinates2D::getNumGridCells() const
{
    return m_totalNumGridCells;
}

int3 GeographicalCoordinates2D::getNumGridCells3d() const
{
    return make_int3(m_numGridCells);
}

float3 GeographicalCoordinates2D::getCellSize() const
{
    return make_float3(m_cellSize);
}

int GeographicalCoordinates2D::getDimension() const
{
    return 2;
}

int GeographicalCoordinates2D::getCartesianDimension() const
{
    return 3;
}

float3 GeographicalCoordinates2D::getAABBMin() const
{
    return make_float3(-m_radius);
}

float3 GeographicalCoordinates2D::getAABBMax() const
{
    return make_float3(m_radius);
}

std::string GeographicalCoordinates2D::getShaderDefine() const
{
    return "GEOGRAPHICAL_COORDINATES_2D";
}

void GeographicalCoordinates2D::setShaderUniforms(mpu::gph::ShaderProgram& shader) const
{
    shader.uniform2f("csInternalData.m_min", glm::vec2(m_min.x,m_min.y));
    shader.uniform2f("csInternalData.m_max", glm::vec2(m_max.x,m_max.y));
    shader.uniform2f("csInternalData.m_size", glm::vec2(m_size.x,m_size.y));
    shader.uniform2f("csInternalData.m_cellSize", glm::vec2(m_cellSize.x,m_cellSize.y));
    shader.uniform2i("csInternalData.m_numGridCells", glm::ivec2(m_numGridCells.x,m_numGridCells.y));
    shader.uniform1i("csInternalData.m_totalNumGridCells", m_totalNumGridCells);
    shader.uniform1f("csInternalData.m_radius", m_radius);
}

