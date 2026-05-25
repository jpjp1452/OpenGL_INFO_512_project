#version 330 core

in vec3 position;

out vec2 UV;

void main()
{
    UV = (position.xy + 1.0) / 2.0;

    vec2 scale = vec2(0.20, 0.03); //size health bar
    vec2 scaledPos = position.xy * scale;

    //center the health bar at the bottom of the screen
    vec2 offset = vec2(0.0, -0.90);
    vec2 finalPos = scaledPos + offset;

    gl_Position = vec4(finalPos, 0.0, 1.0);
}