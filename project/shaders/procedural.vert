#version 330 core

in vec3 inPrevStart;
in vec3 inStart;
in vec3 inEnd;
in float inFactor;
in float inPrevFactor;

out VS_OUT
{
    vec3 prevStart;
    vec3 start;
    vec3 end;
    float factor;
    float prevFactor;
} vs_out;

void main()
{
    vs_out.prevStart = inPrevStart;
    vs_out.start     = inStart;
    vs_out.end       = inEnd;
    vs_out.factor    = inFactor;
    vs_out.prevFactor = inPrevFactor;

    // dummy position
    gl_Position = vec4(inStart, 1.0);
}