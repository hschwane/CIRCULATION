#version 450 core

// input
in float scalar; // scalar value for color

layout(std430) buffer vectorFieldX // ssbo for vector field in x direction
{
    float vecX[];
};
layout(std430) buffer vectorFieldY // ssbo for vector field in y direction
{
    float vecY[];
};

// uniforms
uniform mat4 projectionMat;
uniform mat4 viewMat;
uniform mat4 modelMat;
uniform mat4 modelViewProjectionMat;

uniform vec3 constantColor;
uniform bool scalarColor = false;
uniform float minScalar;
uniform float maxScalar;
uniform vec3 minScalarColor;
uniform vec3 maxScalarColor;

uniform bool prepareVector = false;

uniform float vectorAngle;

// out
out vec3 cellColorGeom;
out vec3 cellColor;
out float angle;

// includes
#include "mathConst.glsl"

// include oordinate system
#if defined(CARTESIAN_COORDINATES_2D)
    #include "coordinateSystems/cartesianCoordinates2D.glsl"
#else if defined(GEOGRAPHICAL_COORDINATES_2D)
    #include "coordinateSystems/geographicalCoordinates2D.glsl"
#endif

void main()
{
#if defined(RENDER_GRID_CELL_POINTS)
    vec3 cellCoord = cs_getCellCoordinate(gl_VertexID);
    vec3 cellCoordCartesian = cs_getCartesian(cellCoord);

    if(cs_getCartesianDimension() == 2)
        gl_Position = modelViewProjectionMat * vec4(cellCoordCartesian.x, 0, cellCoordCartesian.y,1);
    else
        gl_Position = modelViewProjectionMat * vec4(cellCoordCartesian.x, cellCoordCartesian.y, cellCoordCartesian.z, 1);
#else
    gl_Position = vec4(0,0,0,1);
#endif


    float s = scalar;

    if(prepareVector)
    {
        vec2 inVec = vec2(vecX[gl_VertexID],vecY[gl_VertexID]);

        int left = cs_getLeftNeighbor(gl_VertexID);
        int down = cs_getBackwardNeighbor(gl_VertexID);

        if(left > 0)
        {
            inVec.x += vecX[left];
            inVec.x *= 0.5;
        }
        if(down > 0)
        {
            inVec.y += vecY[down];
            inVec.y *= 0.5;
        }

        vec3 vector = cs_getCartesian(vec3(inVec,0));
        float s = length(vector);

        // also figure out vector orientation
        angle = -atan(vector.z, vector.x);
    }

    if(scalarColor)
    {
        float f = (s - minScalar) / (maxScalar - minScalar);
        f = clamp(f,0,1);
        vec3 color = mix(minScalarColor,maxScalarColor,f);

        cellColorGeom = color;
        cellColor = color;
    }
    else
    {
        cellColorGeom = constantColor * float(gl_VertexID) / 3.0f;
        cellColor = constantColor * float(gl_VertexID) / 3.0f;
    }
}
