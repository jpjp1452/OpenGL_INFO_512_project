#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform float size;

out vec2 v_uv;
out vec4 v_col;

void main()
{
    vec3 center = vec3(M[3]);

    // axes caméra (billboard)
    vec3 right = vec3(V[0][0], V[1][0], V[2][0]);
    vec3 up    = vec3(V[0][1], V[1][1], V[2][1]);

    vec3 worldPos =
        center +
        right * aPos.x * size +
        up    * aPos.y * size;

    v_uv = aUV;
    v_col = vec4(1.0); // ou couleur uniforme/attribuée

    gl_Position = P * V * vec4(worldPos, 1.0);
}