#version 330 core

in vec3 position; 
in vec2 tex_coord; 
in vec3 normal; 
uniform float time;


out vec3 v_frag_coord; 
out vec3 v_normal; 
out vec2 v_uv;

uniform mat4 M; 
uniform mat4 itM; 
uniform mat4 V; 
uniform mat4 P; 

void main(){ 
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


    vec4 frag_coord = M*pos;
    gl_Position = P*V*frag_coord; 
    v_normal = vec3(itM * vec4(normal, 0.0)); 
    v_frag_coord = frag_coord.xyz; 
    v_uv = tex_coord; 

    
}