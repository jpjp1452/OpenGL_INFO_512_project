#version 330 core

in vec3 position;
in vec2 tex_coord;
uniform float time;
out vec2 v_uv;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

void main() {
    //apply sin wave to the y coordinate of the vertices depend on the x coordinate and time
    vec4 pos = vec4(position, 1.0);
    float abs_z = 0;
    float moves = 1.0;
    float pivot = 0.0;
    float amplitude = 0.3;
    float frequency = 5.0;
    float offset = 0.0;

  
    abs_z = abs(pivot - pos.x);
    moves = 0.0;


    pos.z += (sin(offset+abs_z + time * frequency) * amplitude)* (abs_z);
    

    gl_Position = P * V * M * pos;
    v_uv = tex_coord;
    
}
