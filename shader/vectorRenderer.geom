#version 450 core

// geometry settings
layout(points) in;
layout(triangle_strip, max_vertices=4) out;

// uniforms
uniform mat4 projectionMat;
uniform mat4 viewMat;
uniform mat4 modelMat;
uniform mat4 modelViewProjectionMat;
uniform float arrowSize;
uniform float gapSize;

// in
in vec3 cellColorGeom[];
in vec3 vectorOrientationCart[]; // orientation of vector

// out
out vec3 cellColor;

// include coordinate system
#if defined(CARTESIAN_COORDINATES_2D)
    #include "coordinateSystems/cartesianCoordinates2D.glsl"
#else if defined(GEOGRAPHICAL_COORDINATES_2D)
    #include "coordinateSystems/geographicalCoordinates2D.glsl"
#endif

// vertices for drawing an arrow
vec4 vertices[] = {
                    vec4(-0.5f,  0.001f, -0.25f,1),
                    vec4(-0.25f, 0.001f,  0.0f, 1),
                    vec4( 0.5f,  0.001f,  0.0f, 1),
                    vec4(-0.5f,  0.001f,  0.25f,1)
                  };

void main()
{
    cellColor = cellColorGeom[0]; // forward color
    vec3 cellCoord = cs_getCellCoordinate(gl_PrimitiveIDIn);

    // calculate new coordinate system of the arrow
    vec3 tangent = vectorOrientationCart[0];
    vec3 normal = normalize(cross(cs_getUnitVectorX(cellCoord), cs_getUnitVectorY(cellCoord)));
    vec3 bitangent = normalize(cross(tangent,normal));

    vec3 pos = cs_getCartesian(cellCoord);

    // generate transformation
    mat4 arrowScale = transpose( mat4( arrowSize,0,0,0,
                                      0,1,0,0,
                                      0,0,arrowSize,0,
                                      0,0,0,1));

    mat4 move = transpose( mat4(  1,0,0,pos.x,
                                  0,1,0,pos.y,
                                  0,0,1,pos.z,
                                  0,0,0,1));

    mat4 rotate = transpose( mat4(  tangent.x, normal.x, bitangent.x, 0,
                                    tangent.y, normal.y, bitangent.y, 0,
                                    tangent.z, normal.z, bitangent.z, 0,
                                    0, 0, 0, 1));

    mat4 transform = move * rotate * arrowScale;// * arrowDirection;

    // calculate vertex positions
    vec4 p1 = transform * vertices[0];
    vec4 p2 = transform * vertices[1];
    vec4 p3 = transform * vertices[2];
    vec4 p4 = transform * vertices[3];

    // emmit vertices
    gl_Position = modelViewProjectionMat * p1;
    EmitVertex();
    gl_Position = modelViewProjectionMat * p2;
    EmitVertex();
    gl_Position = modelViewProjectionMat * p3;
    EmitVertex();
    gl_Position = modelViewProjectionMat * p4;
    EmitVertex();

    EndPrimitive();
}