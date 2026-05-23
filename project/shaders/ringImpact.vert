#version 330 core

layout(location = 0) in vec3 aPos; // quad [-1,1]

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec2 v_uv;

void main()
{
    v_uv = aPos.xy * 0.5 + 0.5;
    gl_Position = P * V * M * vec4(aPos, 1.0);
}