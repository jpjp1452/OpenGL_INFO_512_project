#version 330 core

in vec3 position;
in vec2 tex_coord;
uniform int weaponFrame;

out vec2 textureCoord;

void main(){
    vec2 pos = position.xy;
    gl_Position =vec4(pos, 0.0, 1.0);
    float frameOffset = float(weaponFrame) / 4.0;//offset for each frame in the texture

    textureCoord = vec2(
        tex_coord.x / 4.0 + frameOffset,
        tex_coord.y
    );
}
