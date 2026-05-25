#version 330 core

in vec2 UV;
out vec4 FragColor;

uniform float healthPercent; /* Pourcentage de vie (entre 0.0 et 1.0) */

void main()
{
    vec3 backColor  = vec3(0.1, 0.1, 0.1);  
    vec3 borderColor = vec3(0.9, 0.9, 0.9); 
    
    vec3 lowHealthColor  = vec3(0.8, 0.1, 0.1); // Rouge
    vec3 highHealthColor = vec3(0.1, 0.8, 0.1); // Vert
    

    vec3 healthColor = mix(lowHealthColor, highHealthColor, healthPercent);


    float borderThicknessX = 0.02;
    float borderThicknessY = 0.15;
    
    bool isBorder = (UV.x < borderThicknessX) || (UV.x > 1.0 - borderThicknessX) ||
                    (UV.y < borderThicknessY) || (UV.y > 1.0 - borderThicknessY);

    if (isBorder) 
    {
        FragColor = vec4(borderColor, 1.0);
    }
    else 
    {
        if (UV.x <= healthPercent)
        {
            FragColor = vec4(healthColor, 1.0);
        }
        else
        {
            FragColor = vec4(backColor, 0.8); 
        }
    }
}