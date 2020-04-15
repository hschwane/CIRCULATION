#version 330

layout(location=0) in vec4 inPosition;
layout(location=1) in float scalarField;

out vec4 color; // this is passed to the fragment shader

uniform float minScalar;
uniform float maxScalar;
uniform vec3 minScalarColor;
uniform vec3 maxScalarColor;

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
        float s = scalarField;
        // apply transfer function to the density
        float f = (s - minScalar) / (maxScalar - minScalar);
        f = clamp(f,0,1);
        color = vec4(mix(minScalarColor,maxScalarColor,f),1.0f);
    }
}