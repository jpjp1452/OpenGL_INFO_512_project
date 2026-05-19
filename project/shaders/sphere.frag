#version 330 core

out vec4 FragColor;

in vec3 v_worldPos;
in vec3 v_normal;

uniform vec3 baseColor;
uniform vec3 view_pos;
uniform float time;

// 0 = noyau soleil, 1 = halo
uniform int isHalo;
uniform float haloIntensity;

void main()
{https://tools.wwwtyro.net/space-3d/index.html#animationSpeed=1&fov=80&nebulae=true&pointStars=true&resolution=1024&seed=7jbalciyna40&stars=true&sun=true
    vec3 N = normalize(v_normal);
    vec3 V = normalize(view_pos - v_worldPos);

    if (isHalo == 1)
    {
        float rim = pow(1.0 - max(dot(N, V), 0.0), 2.4);
        float pulse = 0.9 + 0.1 * sin(time * 0.8);
        vec3 glow = baseColor * (0.7 + 1.2 * rim) * haloIntensity * pulse;
        float alpha = clamp(0.12 + 0.45 * rim, 0.0, 0.75);
        FragColor = vec4(glow, alpha);
        return;
    }

    // Noyau solaire (émissif, plasma)
    float p1 = sin(time * 1.2 + v_worldPos.x * 10.0) * 0.5 + 0.5;
    float p2 = sin(time * 1.7 + v_worldPos.y * 13.0) * 0.5 + 0.5;
    float p3 = sin(time * 1.3 + v_worldPos.z * 11.0) * 0.5 + 0.5;
    float plasma = (p1 + p2 + p3) / 3.0;

    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);

    vec3 coreColor = baseColor * (1.5 + plasma * 1.0);
    vec3 rimColor = vec3(1.0, 0.55, 0.15) * rim * 1.7;

    FragColor = vec4(coreColor + rimColor, 1.0);
}