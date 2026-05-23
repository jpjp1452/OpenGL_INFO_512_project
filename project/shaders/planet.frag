#version 330 core

out vec4 FragColor;

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_texCoord;

uniform sampler2D planetTexture;
uniform vec3 light_pos;
uniform vec3 u_view_pos;
uniform float time;

void main()
{
    vec3 N = normalize(v_normal);
    vec3 L = normalize(light_pos - v_worldPos);
    vec3 V = normalize(u_view_pos - v_worldPos);
    vec3 R = reflect(-L, N);
    
    // Load Planet texture
    vec3 texColor = texture(planetTexture, v_texCoord).rgb;
    
    // Apply reddish Planet tint
    texColor = mix(texColor, texColor * vec3(1.2, 0.7, 0.5), 0.4);
    
    // Lighting
    float ambient = 0.15;
    float diffuse = max(dot(N, L), 0.0);
    float specular = pow(max(dot(R, V), 0.0), 8.0) * 0.15;
    
    // Slight atmospheric glow on edges (Fresnel)
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 2.0) * 0.2;
    
    vec3 finalColor = texColor * (ambient + 0.9 * diffuse) + vec3(1.0) * specular + vec3(0.8, 0.4, 0.2) * fresnel;
    
    FragColor = vec4(finalColor, 1.0);
}