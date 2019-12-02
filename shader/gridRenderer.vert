#version 450 core

// uniforms
uniform mat4 projectionMat;
uniform mat4 viewMat;
uniform mat4 viewProjectionMat;

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
        gl_Position = viewProjectionMat * vec4(cellCoordCartesian.x, 0, cellCoordCartesian.y,1);
    else
        gl_Position = viewProjectionMat * vec4(cellCoordCartesian.x, cellCoordCartesian.y, cellCoordCartesian.z, 1);
#else
    gl_Position = vec4(0,0,0,1);
#endif
}
