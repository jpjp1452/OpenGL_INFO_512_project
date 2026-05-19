#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D asteroidTexture;
uniform int useTexture;

void main() {
	FragColor = texture(asteroidTexture, TexCoords);
}