#version 330 core

in vec3 fragColor;
in float factor;
out vec4 outColor;

void main()
{
    vec4 brownColor = vec4(0.55, 0.27, 0.07, 1.0);
    vec4 greenColor = vec4(0.0, 0.5, 0.0, 1.0);

    vec4 finalColor = mix( greenColor,brownColor, factor);

    outColor = vec4(finalColor.rgb, 1.0);
}