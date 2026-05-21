#version 330 core

in vec3 fragColor;
in float factor;
out vec4 outColor;

void main()
{

    outColor = vec4(fragColor, 1.0);
}