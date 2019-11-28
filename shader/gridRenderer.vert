#version 450 core

uniform mat4 projectionMat;
uniform mat4 viewMat;
uniform mat4 viewProjectionMat;

void main()
{
    gl_Position = viewProjectionMat * vec4(vec3(0.0), 1.0);
}
