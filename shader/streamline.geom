#version 450 core

// geometry settings
layout(points) in;
layout(line_strip, max_vertices=100) out;

// ssbo data
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

uniform float slLength;
uniform float dx;

// out
out vec3 cellColor; // actually the vertex color on the line

// include coordinate system
#if defined(CARTESIAN_COORDINATES_2D)
    #include "coordinateSystems/cartesianCoordinates2D.glsl"
#else if defined(GEOGRAPHICAL_COORDINATES_2D)
    #include "coordinateSystems/geographicalCoordinates2D.glsl"
#endif

float linearInterpolate(float targetPosition, float positionA, float valueA, float positionB, float valueB)
{
    float f = (targetPosition-positionA) / (positionB-positionA);
    return valueA * (1-f) + valueB * f;
}

void main()
{
    cellColor = vec3(1);
    vec2 position = gl_in[0].gl_Position.xy;
    vec2 prev = vec2(0,0);

    float lineLength = 0;
    int nVertex = 0;
    while(lineLength < slLength && nVertex < 100)
    {
        if(position.x < cs_getMinCoord().x || position.y < cs_getMinCoord().y || position.x > cs_getMaxCoord().x || position.y > cs_getMaxCoord().y)
            break;

        int cellId = cs_getCellId(vec3(position,0));
        vec2 vector = vec2(vecX[cellId],vecY[cellId]);

        int left = cs_getLeftNeighbor(cellId);
        int down = cs_getBackwardNeighbor(cellId);
        if(left > 0)
        {
            float posX1 = cs_getCellCoordinate(cellId).x - 0.5*cs_getCellSize().x;
            float posX2 = cs_getCellCoordinate(cellId).x + 0.5*cs_getCellSize().x;
            vector.x = linearInterpolate(position.x,posX1, vecX[left],posX2, vector.x);
        }
        if(down > 0)
        {
            float posY1 = cs_getCellCoordinate(cellId).y - 0.5*cs_getCellSize().y;
            float posY2 = cs_getCellCoordinate(cellId).y + 0.5*cs_getCellSize().y;
            vector.y = linearInterpolate(position.y,posY1, vecY[left],posY2, vector.y);
        }

        float l = length(vector);

        if(l < 0.0001*cs_getCellSize().x || dot(vector,prev)<0)
            break;

        vector = vector / l;
        prev = vector;

        vec2 change = vector * dx;
        position += change;

        // emmit vertex
        gl_Position = modelViewProjectionMat * vec4( 1.001*cs_getCartesian(vec3(position,1)),1);
        EmitVertex();

        nVertex ++;
        lineLength += l;
    }

    EndPrimitive();
}