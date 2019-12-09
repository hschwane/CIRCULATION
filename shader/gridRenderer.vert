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

uniform bool vectorColor = false;
uniform float minVecLength;
uniform float maxVecLength;
uniform vec3 minVecColor;
uniform vec3 maxVecColor;

uniform float vectorAngle;

// out
out vec3 cellColorGeom;
out vec3 cellColor;
out float angle;

// include oordinate system
#if defined(CARTESIAN_COORDINATES_2D)
    #include "coordinateSystems/cartesianCoordinates2D.glsl"
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

    if(scalarColor)
    {
        float f = (scalar - minScalar) / (maxScalar - minScalar);
        f = clamp(f,0,1);
        vec3 color = mix(minScalarColor,maxScalarColor,f);

        cellColorGeom = color;
        cellColor = color;
    }
    else if(vectorColor)
    {
        vec2 inVec = vec2(vecX[gl_VertexID],vecY[gl_VertexID]);
        vec2 vector = cs_getCartesian(vec3(inVec,0)).xy;

        float s = length(vector);
        float f = (s - minVecLength) / (maxVecLength - minVecLength);
        f = clamp(f,0,1);
        vec3 color = mix(minVecColor,maxVecColor,f);

        cellColorGeom = color;
        cellColor = color;

        // also figure out vector orientation
        angle = atan(vector.y, vector.x);
    }
    else
    {
        cellColorGeom = constantColor;
        cellColor = constantColor;
    }
}
