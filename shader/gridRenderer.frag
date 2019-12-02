#version 450 core

in vec3 cellColor;
out vec4 fragColor;

void main()
{
    fragColor = vec4(cellColor, 1.0);
}
