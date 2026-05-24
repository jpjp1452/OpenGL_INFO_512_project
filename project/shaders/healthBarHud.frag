#version 330 core

in vec2 UV;
out vec4 FragColor;

uniform float healthPercent; /* Pourcentage de vie (entre 0.0 et 1.0) */

void main()
{
    // --- Configuration des couleurs ---
    vec3 backColor  = vec3(0.1, 0.1, 0.1);  // Fond vide de la barre (gris foncé)
    vec3 borderColor = vec3(0.9, 0.9, 0.9); // Bordure blanche/grisée
    
    vec3 lowHealthColor  = vec3(0.8, 0.1, 0.1); // Rouge
    vec3 highHealthColor = vec3(0.1, 0.8, 0.1); // Vert
    
    // La couleur varie du rouge au vert en fonction de la vie restante
    vec3 healthColor = mix(lowHealthColor, highHealthColor, healthPercent);

    // --- Calcul de la bordure ---
    // Les valeurs X et Y sont différentes pour que la bordure soit uniforme visuellement 
    // malgré la forme rectangulaire de la barre
    float borderThicknessX = 0.02;
    float borderThicknessY = 0.15;
    
    // Si on est sur les extrêmités du quad, c'est une bordure
    bool isBorder = (UV.x < borderThicknessX) || (UV.x > 1.0 - borderThicknessX) ||
                    (UV.y < borderThicknessY) || (UV.y > 1.0 - borderThicknessY);

    if (isBorder) 
    {
        FragColor = vec4(borderColor, 1.0);
    }
    else 
    {
        // Intérieur de la barre : on remplit si le point UV courant (de 0 à 1) est inférieur ou égal à la vie
        if (UV.x <= healthPercent)
        {
            FragColor = vec4(healthColor, 1.0);
        }
        else
        {
            // Espace vide
            FragColor = vec4(backColor, 0.8); // 0.8 d'alpha pour une légère transparence
        }
    }
}