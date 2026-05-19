#version 330 core

out vec4 FragColor;

in vec3 v_worldPos;
in vec3 v_normal;

uniform vec3 baseColor;
uniform vec3 light_pos;
uniform vec3 view_pos;


void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(light_pos - v_worldPos);
    vec3 V = normalize(view_pos - v_worldPos);
    vec3 R = reflect(-L, N);

    // Lighting components
    float ambient = 0.3;
    float diffuse = max(dot(N, L), 0.0);
    float specular = pow(max(dot(R, V), 0.0), 32.0) * 0.6;

    // Combine lighting
    vec3 color = baseColor * (ambient + 0.7 * diffuse) + vec3(1.0) * specular;
    
    FragColor = vec4(color, 10.9);
}
