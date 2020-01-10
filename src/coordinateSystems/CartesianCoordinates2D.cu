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

// function definitions of the CartesianCoordinates2D class
//-------------------------------------------------------------------
CartesianCoordinates2D::CartesianCoordinates2D(float3 min, float3 max, int3 numGridCells)
    : m_min(make_float2(min)), m_max(make_float2(max)),
    m_numGridCells(make_int2(numGridCells)),
    m_totalNumGridCells(numGridCells.x*numGridCells.y),
    m_size(m_max-m_min),
    m_cellSize( m_size / make_float2( (m_numGridCells.x<2) ? 1 : m_numGridCells.x-1, (m_numGridCells.y<2) ? 1 : m_numGridCells.y-1))
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

float3 CartesianCoordinates2D::getUnitVectorX(float3 position) const
{
    return make_float3(1.0f,0.0f,0.0f);
}

float3 CartesianCoordinates2D::getUnitVectorY(float3 position) const
{
    return make_float3(0.0f,1.0f,0.0f);
}

float3 CartesianCoordinates2D::getUnitVectorZ(float3 position) const
{
    return make_float3(0.0f,0.0f,1.0f);
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

int3 CartesianCoordinates2D::getCellId3d(int cellId) const
{
    return int3{cellId%m_numGridCells.x, cellId/m_numGridCells.x,0};
}

int CartesianCoordinates2D::getCellId(const float3& coord) const
{
    int3 cellId3d = getCellId3d(coord);
    return cellId3d.y*m_numGridCells.x+ cellId3d.x;
}

int CartesianCoordinates2D::getCellId(const int3& cellId3d) const
{
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

int CartesianCoordinates2D::getNumGridCells() const
{
    return m_totalNumGridCells;
}

int3 CartesianCoordinates2D::getNumGridCells3d() const
{
    return make_int3(m_numGridCells,1);
}

int3 CartesianCoordinates2D::hasBoundary() const
{
    return make_int3(1,1,0);
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

std::string CartesianCoordinates2D::getShaderDefine() const
{
    return "CARTESIAN_COORDINATES_2D";
}

void CartesianCoordinates2D::setShaderUniforms(mpu::gph::ShaderProgram& shader) const
{
    shader.uniform2f("csInternalData.m_min", glm::vec2(m_min.x,m_min.y));
    shader.uniform2f("csInternalData.m_max", glm::vec2(m_max.x,m_max.y));
    shader.uniform2f("csInternalData.m_size", glm::vec2(m_size.x,m_size.y));
    shader.uniform2f("csInternalData.m_cellSize", glm::vec2(m_cellSize.x,m_cellSize.y));
    shader.uniform2i("csInternalData.m_numGridCells", glm::ivec2(m_numGridCells.x,m_numGridCells.y));
    shader.uniform1i("csInternalData.m_totalNumGridCells", m_totalNumGridCells);
}

CSType CartesianCoordinates2D::getType()
{
    return CSType::cartesian2d;
}
