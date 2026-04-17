#version 400 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;

out VS_OUT {
    vec3 pos;
    vec2 texCoord;
} vs_out;

uniform mat4 M;


void main()
{
    vs_out.pos = vec3(M * vec4(position, 1.0));
    vs_out.texCoord = tex_coord;
    gl_Position = vec4(vs_out.pos, 1.0);
}
