#version 450 core

// geometry settings
layout(points) in;
layout(triangle_strip, max_vertices=4) out;

// uniforms
uniform mat4 projectionMat;
uniform mat4 viewMat;
uniform mat4 modelMat;
uniform mat4 modelViewProjectionMat;

uniform float gapSize;

// in
in vec3 cellColorGeom[];

// out
out vec3 cellColor;

// include coordinate system
#if defined(CARTESIAN_COORDINATES_2D)
    #include "coordinateSystems/cartesianCoordinates2D.glsl"
#else if defined(GEOGRAPHICAL_COORDINATES_2D)
    #include "coordinateSystems/geographicalCoordinates2D.glsl"
#endif

void main()
{
    vec3 cellCoord = cs_getCellCoordinate(gl_PrimitiveIDIn);
    vec3 vertexCoord;

    cellColor = cellColorGeom[0];

    // generate local positions
    vertexCoord = cellCoord + vec3(-0.5,0.5, 0.5) * cs_getCellSize();
    vec3 corner1 = cs_getCartesian(vertexCoord);

    vertexCoord = cellCoord + vec3(-0.5,-0.5, 0.5) * cs_getCellSize();
    vec3 corner2 = cs_getCartesian(vertexCoord);

    vertexCoord = cellCoord + vec3(0.5,0.5, 0.5) * cs_getCellSize();
    vec3 corner3 = cs_getCartesian(vertexCoord);

    vertexCoord = cellCoord + vec3(0.5,-0.5, 0.5) * cs_getCellSize();
    vec3 corner4 = cs_getCartesian(vertexCoord);

    // move corners along diagonal
    vec3 diag1 = normalize(corner1-corner4);
    corner4 += diag1 * gapSize;
    corner1 -= diag1 * gapSize;

    vec3 diag2 = normalize(corner2-corner3);
    corner3 += diag2 * gapSize;
    corner2 -= diag2 * gapSize;

    // emmit vertices
    gl_Position = modelViewProjectionMat * vec4(corner1,1);
    EmitVertex();
    gl_Position = modelViewProjectionMat * vec4(corner2,1);
    EmitVertex();
    gl_Position = modelViewProjectionMat * vec4(corner3,1);
    EmitVertex();
    gl_Position = modelViewProjectionMat * vec4(corner4,1);
    EmitVertex();

    EndPrimitive();
}