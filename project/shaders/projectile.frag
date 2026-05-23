#version 330 core

out vec4 FragColor;

in vec3 v_worldPos;
in vec3 v_normal;

uniform vec3 baseColor;
uniform vec3 view_pos;
uniform float time;

void main()
{
    vec3 N = normalize(v_normal);
    vec3 V = normalize(view_pos - v_worldPos);

    // Rim view dependent
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    float pulse = 0.7 + 0.3 * sin(time * 20.0);
    vec3 core = baseColor * (1.2 + pulse);
    vec3 glow = baseColor * rim * 2.0;
    float intensity = 0.6 + rim * 1.4;

    float d = length(gl_PointCoord - vec2(0.5));
    float beam = exp(-d * 8.0);

    vec3 color = baseColor * beam * 3.0 + glow;
    color = color / (color + vec3(1.0)); // tone mapping
    FragColor = vec4(color, 1.0);
}