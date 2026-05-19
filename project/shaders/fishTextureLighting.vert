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
    float abs_z = 1.0;
    float moves = 1.0;
    float pivot = 0.0;
    float amplitude = 0.7;
    float frequency = 5.0;
    float offset = 0.0;

    abs_z = abs(pivot - pos.x);
    moves = 0.0;



        if (pos.z > 0.0)
        {
            moves = 1.0;
        }
        else
        {
            moves = -1.0;
        }
    



    if(pos.y < -0.25)
    {   
 
            if (abs(pos.z) <0.13 || pos.y < -0.4)
            {
                abs_z = abs(pos.y - -0.25);
                pos.x += (sin( time * frequency) * amplitude)* (abs_z)*moves;
            }
    }
    




    vec4 frag_coord = M*pos;
    gl_Position = P*V*frag_coord; 
    v_normal = vec3(itM * vec4(normal, 0.0)); 
    v_frag_coord = frag_coord.xyz; 
    v_uv = tex_coord; 

    
}