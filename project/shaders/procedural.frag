#version 330 core

in vec3 fragColor;
in float factor;
out vec4 outColor;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float leafStartFactor;
        
in vec2 v_uv;

void main()
{


    if (factor < leafStartFactor) {
        vec3 color1 = texture(texture2, v_uv).rgb;
        outColor = vec4(color1, 1.0);
    }
    else {
        vec3 color2 = texture(texture1, v_uv).rgb;
        outColor = vec4(color2, 1.0);
    }




}