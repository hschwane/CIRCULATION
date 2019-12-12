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
in float angle[]; // angle of the vector

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

    // generate local positions
    vec3 corner1 = cellCoord + vec3(-0.5,0.5, 0.5) * cs_getCellSize();
    vec3 corner2 = cellCoord + vec3(-0.5,-0.5, 0.5) * cs_getCellSize();
    vec3 corner3 = cellCoord + vec3(0.5,0.5, 0.5) * cs_getCellSize();
    vec3 corner4 = cellCoord + vec3(0.5,-0.5, 0.5) * cs_getCellSize();
    corner1.y = clamp(corner1.y,cs_getMinCoord().y,cs_getMaxCoord().y);
    corner2.y = clamp(corner2.y,cs_getMinCoord().y,cs_getMaxCoord().y);
    corner3.y = clamp(corner3.y,cs_getMinCoord().y,cs_getMaxCoord().y);
    corner4.y = clamp(corner4.y,cs_getMinCoord().y,cs_getMaxCoord().y);

    // move corners along diagonal
    vec3 diag1 = normalize(corner1-corner4);
    corner4 += diag1 * gapSize;
    corner1 -= diag1 * gapSize;
    vec3 diag2 = normalize(corner2-corner3);
    corner3 += diag2 * gapSize;
    corner2 -= diag2 * gapSize;

    // convert to cartesian
    corner1 = cs_getCartesian(corner1);
    corner2 = cs_getCartesian(corner2);
    corner3 = cs_getCartesian(corner3);
    corner4 = cs_getCartesian(corner4);

    // figure out position and orientation of arrow
    float upperEdge = length(corner3-corner1);
    float lowerEdge = length(corner4-corner2);
    float side = length(corner1 - corner2);
    float a = min(upperEdge,lowerEdge);
    float b = max(upperEdge,lowerEdge);
    float h = sqrt(side*side + (b-a)*(b-a));
    float y = ( (b+2*a) / (3*(a+b)) ) *h;

    vec3 tangent = normalize(corner3-corner1);
    vec3 normal = normalize(cross(corner2-corner1, tangent));
    vec3 bitangent = normalize(cross(tangent,normal));

    vec3 pos =  corner2 + tangent*lowerEdge*0.5f - bitangent*y;

    // generate transformation
    mat4 arrowScale = transpose( mat4( arrowSize,0,0,0,
                                      0,1,0,0,
                                      0,0,arrowSize,0,
                                      0,0,0,1));

    mat4 arrowDirection = transpose( mat4( cos(angle[0]), 0, sin(angle[0]), 0,
                                           0,1,0,0,
                                           -sin(angle[0]), 0, cos(angle[0]), 0,
                                           0,0,0,1));

    mat4 move = transpose( mat4(  1,0,0,pos.x,
                                  0,1,0,pos.y,
                                  0,0,1,pos.z,
                                  0,0,0,1));

    mat4 rotate = transpose( mat4(  tangent.x, normal.x, bitangent.x, 0,
                                    tangent.y, normal.y, bitangent.y, 0,
                                    tangent.z, normal.z, bitangent.z, 0,
                                    0, 0, 0, 1));

    mat4 transform = move * rotate * arrowScale * arrowDirection;

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