#version 330 core

in vec2 v_uv;
out vec4 FragColor;

uniform sampler2D texture1;

void main() {
    vec3 color = texture(texture1, v_uv).rgb;
    FragColor = vec4(color, 1.0);
}
