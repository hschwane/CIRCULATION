#version 330

layout(location=0) in vec4 inPosition;
layout(location=1) in float inDensity;

out vec4 color; // this is passed to the fragment shader

uniform float maxDensity; // has the same value for all vertices

uniform bool useConstColor; // should constant color be used
uniform vec3 constColor; // the constant color to use

uniform mat4 modelMat; // matrix to go from object space to world space
uniform mat4 viewMat; // matrix to go from world space to camera space
uniform mat4 projectionMat; // matrix to perform perspective projection
uniform mat4 modelViewProjectionMat; // combined mvp

void main()
{
    gl_Position = modelViewProjectionMat * vec4(inPosition.xyz,1);

    if(useConstColor)
    {
        color = vec4(constColor,1.0);
    }
    else
    {
        // apply transfer function to the density
        color = vec4(inDensity / maxDensity, inDensity / maxDensity, inDensity/ maxDensity, 1);
    }
}