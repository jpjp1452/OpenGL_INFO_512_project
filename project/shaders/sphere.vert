#version 330 core

in vec3 position;
in vec3 normal;
uniform float time;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 v_worldPos;
out vec3 v_normal;

void main() {
    vec4 pos = vec4(position, 1.0);

    // Create surface waves by displacing along the normal (radial direction)
    vec3 sphereNormal = normalize(position);
    
    // Wave patterns that propagate across the surface using position coordinates
    float wave1 = sin(time * 0.64 + position.x * 3.0 + position.y * 2.0) * 0.28;
    float wave2 = sin(time * 1.8 + position.y * 3.0 - position.z * 2.0) * 0.26;
    float wave3 = sin(time * 0.92 + position.z * 2.5 + position.x * 1.5) * 0.15;
    
    float totalWave = wave1 + wave2 + wave3;
    
    // Displace the vertex along the surface normal
    pos.xyz += sphereNormal * totalWave;

    vec4 worldPos = M * pos;
    v_worldPos = worldPos.xyz;

    vec3 localNormal = normalize(pos.xyz);
    mat3 normalMatrix = mat3(transpose(inverse(M)));
    v_normal = normalize(normalMatrix * localNormal);

    gl_Position = P * V * worldPos;
}
