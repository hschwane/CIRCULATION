#version 450 core

uniform mat4 projectionMat;
uniform mat4 viewMat;
uniform mat4 viewProjectionMat;

#if defined(CARTESIAN_COORDINATES_2D)
    #include "coordinateSystems/cartesianCoordinates2D.glsl"
#endif

void main()
{
    vec3 cellCoord = cs_getCellCoordinate(gl_VertexID);
    vec3 cellCoordCartesian = cs_getCartesian(cellCoord);

    if(cs_getCartesianDimension() == 2)
        gl_Position = projectionMat * viewMat * vec4(cellCoordCartesian.x, 0, cellCoordCartesian.y,1);
    else
        gl_Position = projectionMat * viewMat * vec4(cellCoordCartesian.x, cellCoordCartesian.y, cellCoordCartesian.z, 1);
}
