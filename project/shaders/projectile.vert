#version 330 core

in vec3 position;
in vec3 normal;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 v_worldPos;
out vec3 v_normal;

void main()
{
    vec4 worldPos = M * vec4(position, 1.0);
    v_worldPos = worldPos.xyz;

    mat3 normalMatrix = mat3(transpose(inverse(M)));
    v_normal = normalize(normalMatrix * normal);

    gl_Position = P * V * worldPos;
}