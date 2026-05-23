#version 330 core

in vec2 v_uv;
out vec4 FragColor;

uniform vec3 color;
uniform float alpha;

void main()
{
    float d = length(v_uv - vec2(0.5));

    float ring = smoothstep(0.5, 0.45, d) - smoothstep(0.45, 0.35, d);

    FragColor = vec4(color, ring * alpha);
}