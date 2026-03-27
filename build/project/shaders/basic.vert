#version 330 core

in vec3 position;
in vec3 color;

out vec4 v_col;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

void main(){
    gl_Position = P * V * M * vec4(position, 1.0);
    v_col = vec4(color, 1.0);
}
