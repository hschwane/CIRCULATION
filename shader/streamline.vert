#version 450 core

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

    // http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
    float g = 1.32471795724474602596;
    float a1 = 1.0/g;
    float a2 = 1.0/(g*g);
    float a3 = 1.0/(g*g*g);

    vec2 randomPoint;
    randomPoint.x = mod(0.5+a1*gl_VertexID,1);
    randomPoint.y = mod(0.5+a2*gl_VertexID,1);

    // map into range of the coordinate system
    gl_Position = vec4( cs_getMinCoord().xy + randomPoint.xy * ( cs_getMaxCoord().xy - cs_getMinCoord().xy ) ,0,1);
}
