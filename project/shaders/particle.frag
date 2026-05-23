#version 330 core

in vec2 v_uv;
in vec4 v_col;

out vec4 FragColor;

void main()
{
    float d = distance(v_uv, vec2(0.5));

    // noyau + halo
    float core = smoothstep(0.15, 0.0, d);
    float glow = smoothstep(0.5, 0.15, d);

    float intensity = core * 3.0 + glow;

    vec3 color = vec3(1.0, 0.6, 0.2) * intensity;

    float alpha = glow;

    FragColor = vec4(color, alpha);
}