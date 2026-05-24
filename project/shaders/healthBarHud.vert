#version 330 core

in vec3 position;

out vec2 UV;

void main()
{
    // On convertit la position [-1, 1] en coordonnées UV [0, 1] pour le fragment shader
    UV = (position.xy + 1.0) / 2.0;

    // ----- ÉTAPE 1 : Échelle (Taille de la barre) -----
    // On réduit la taille à 20% de la largeur de l'écran et 3% de la hauteur
    vec2 scale = vec2(0.20, 0.03); 
    vec2 scaledPos = position.xy * scale;

    // ----- ÉTAPE 2 : Positionnement -----
    // En coordonnées normalisées (NDC), l'écran va de -1 (gauche/bas) à +1 (droite/haut).
    // On la place en bas à droite avec une petite marge.
    vec2 offset = vec2(0.0, -0.90);
    vec2 finalPos = scaledPos + offset;

    gl_Position = vec4(finalPos, 0.0, 1.0);
}