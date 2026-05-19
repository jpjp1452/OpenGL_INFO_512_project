#version 330 core

in vec3 position;
in vec2 tex_coord;
in vec3 normal;

layout (location = 3) in mat4 instanceMatrix;

out vec2 TexCoords;

uniform mat4 P;
uniform mat4 V;

void main() {
    gl_Position = P * V * instanceMatrix * vec4(position, 1.0);
    TexCoords = tex_coord;
}