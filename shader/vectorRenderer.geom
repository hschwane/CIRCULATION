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
in float angle[]; // angle of the vector

// out
out vec3 cellColor;

// include coordinate system
#if defined(CARTESIAN_COORDINATES_2D)
    #include "coordinateSystems/cartesianCoordinates2D.glsl"
#endif

// vertices for drawing an arrow
vec4 vertices[] = {
                    vec4(-0.25f,  -0.5f, 0, 1),
                    vec4(0.0f,    -0.25f, 0, 1),
                    vec4(0.0f,    0.5f, 0, 1),
                    vec4(0.25f, -0.5f, 0, 1)
                  };

vec4 switchPlaneFor2D(vec4 cartesianPosition)
{
    return vec4(cartesianPosition.x, 0, cartesianPosition.y, 1);
}

void main()
{
    vec3 cellCoord = cs_getCellCoordinate(gl_PrimitiveIDIn);
    vec3 cellCoordCart = cs_getCartesian(cellCoord);

    cellColor = cellColorGeom[0]; // forward color

    mat4 move = mat4( 1,0,0,cellCoordCart.x,
                      0,1,0,cellCoordCart.y,
                      0,0,1,cellCoordCart.z,
                      0,0,0,1);
    mat4 rotate = mat4( cos(angle[0]), -sin(angle[0]), 0, 0,
                        sin(angle[0]), cos(angle[0]), 0, 0,
                        0,0,0,0,
                        0,0,0,1);
    mat4 transform = move * rotate;

    // calculate vertex positions
    vec4 p1 = transform * vertices[0];
    vec4 p2 = transform * vertices[1];
    vec4 p3 = transform * vertices[2];
    vec4 p4 = transform * vertices[3];

    // emmit vertices
    gl_Position = modelViewProjectionMat * switchPlaneFor2D(p1);
    EmitVertex();
    gl_Position = modelViewProjectionMat * switchPlaneFor2D(p2);
    EmitVertex();
    gl_Position = modelViewProjectionMat * switchPlaneFor2D(p3);
    EmitVertex();
    gl_Position = modelViewProjectionMat * switchPlaneFor2D(p4);
    EmitVertex();

    EndPrimitive();
}