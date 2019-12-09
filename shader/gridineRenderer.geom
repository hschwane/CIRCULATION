#version 450 core

// geometry settings
layout(points) in;
layout(line_strip, max_vertices=5) out;

// uniforms
uniform mat4 projectionMat;
uniform mat4 viewMat;
uniform mat4 modelMat;
uniform mat4 modelViewProjectionMat;

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

    // Vertex 1
    vertexCoord = cellCoord + vec3(0.5,0.5,0.5) * cs_getCellSize();
    gl_Position = modelViewProjectionMat * vec4( cs_getCartesian(vertexCoord),1 );
    EmitVertex();

    // Vertex 2
    vertexCoord = cellCoord + vec3(0.5,-0.5,0.5) * cs_getCellSize();
    gl_Position = modelViewProjectionMat * vec4( cs_getCartesian(vertexCoord),1 );
    EmitVertex();

    // Vertex 3
    vertexCoord = cellCoord + vec3(-0.5,-0.5,0.5) * cs_getCellSize();
    gl_Position = modelViewProjectionMat * vec4( cs_getCartesian(vertexCoord),1 );
    EmitVertex();

    // Vertex 4
    vertexCoord = cellCoord + vec3(-0.5,0.5,0.5) * cs_getCellSize();
    gl_Position = modelViewProjectionMat * vec4( cs_getCartesian(vertexCoord),1 );
    EmitVertex();

    // Vertex 5 (vertex 1 again to close the loop)
    vertexCoord = cellCoord + vec3(0.5,0.5,0.5) * cs_getCellSize();
    gl_Position = modelViewProjectionMat * vec4( cs_getCartesian(vertexCoord),1 );
    EmitVertex();

    EndPrimitive();
}