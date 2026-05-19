#version 330 core

in vec2 position;
in vec2 tex_coord;
uniform int weaponFrame;

out vec2 textureCoord;

void main(){
    vec2 pos = position;
    gl_Position =vec4(pos, 0.0, 1.0);
    float frameOffset = float(weaponFrame) / 4.0;

    textureCoord = vec2(
        tex_coord.x / 4.0 + frameOffset,
        tex_coord.y
    );
}
