#include "mathConst.glsl"

struct GeographicalCoordinates2D_internal
{
    float m_radius;
    vec2 m_min;
    vec2 m_max;
    vec2 m_size;
    vec2 m_cellSize;
    ivec2 m_numGridCells;
    int m_totalNumGridCells;
};

uniform GeographicalCoordinates2D_internal csInternalData;

vec3 cs_getCartesian(const vec3 coord)
{
    float phi = PI*0.5 - coord.y;
    float sinPhi = sin(phi);
    return vec3( csInternalData.m_radius * cos(coord.x) * sinPhi,
                 csInternalData.m_radius * sin(coord.x) * sinPhi,
                 csInternalData.m_radius * cos(phi));
}

vec3 cs_getCoord(const vec3 cartesian)
{
    float r = sqrt( cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z );
    float phi = acos(cartesian.z / r);
    return vec3(  atan(cartesian.y,cartesian.x), PI*0.5 - phi, 0);
}

vec3 cs_getCellCoordinate3d(const ivec3 cellId3d)
{
    ivec2 cellId2d = ivec2(cellId3d);
    vec2 coord2d = vec2(cellId2d) * csInternalData.m_cellSize + csInternalData.m_min;
    return vec3(coord2d,0);
}

vec3 cs_getCellCoordinate(int cellId)
{
    return cs_getCellCoordinate3d( ivec3(cellId%csInternalData.m_numGridCells.x, cellId/csInternalData.m_numGridCells.x,0));
}

ivec3 cs_getCellId3d(const vec3 coord)
{
    vec2 coord2d =  (vec2(coord) - csInternalData.m_min) / csInternalData.m_cellSize;
    return ivec3(roundEven(coord2d.x),roundEven(coord2d.y),0);
}

int cs_getCellId(const vec3 coord)
{
    ivec3 cellId3d = cs_getCellId3d(coord);
    return cellId3d.y*csInternalData.m_numGridCells.x+ cellId3d.x;
}

int cs_getRightNeighbor(int cellId)
{
    return cellId+1;
}

int cs_getLeftNeighbor(int cellId)
{
    return cellId-1;
}

int cs_getForwardNeighbor(int cellId)
{
    return cellId+csInternalData.m_numGridCells.x;
}

int cs_getBackwardNeighbor(int cellId)
{
    return cellId-csInternalData.m_numGridCells.x;
}

int cs_getUpNeighbor(int cellId)
{
    return -1;
}

int cs_getDownNeighbor(int cellId)
{
    return -1;
}

int cs_getNumGridCells()
{
    return csInternalData.m_totalNumGridCells;
}

ivec3 cs_getNumGridCells3d()
{
    return ivec3(csInternalData.m_numGridCells,0);
}

vec3 cs_getCellSize()
{
    return vec3(csInternalData.m_cellSize,0);
}

vec3 cs_getMinCoord()
{
    return vec3(csInternalData.m_min.x, csInternalData.m_min.y, 0);
}

vec3 cs_getMaxCoord()
{
    return vec3(csInternalData.m_max.x,csInternalData.m_max.y,0);
}

int cs_getDimension()
{
    return 2;
}

int cs_getCartesianDimension()
{
    return 3;
}

vec3 cs_getAABBMin()
{
    return vec3(-csInternalData.m_radius);
}

vec3 cs_getAABBMax()
{
    return vec3(csInternalData.m_radius);
}
