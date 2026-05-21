#version 330 core

layout(points) in;

layout(triangle_strip, max_vertices = 64) out;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

uniform float leafStartFactor;
uniform float leafAmplification;

in VS_OUT
{
    vec3 prevStart;
    vec3 start;
    vec3 end;
    float factor;
    float prevFactor;
} gs_in[];

out vec3 fragColor;
out float factor;
const int SIDES = 8;
out vec2 v_uv;

void emitVertex(vec3 p, float colorFactor, vec2 uv)
{
    gl_Position =
        P *
        V *
        M *
        vec4(p, 1.0);



    vec4 brownColor = vec4(0.55, 0.27, 0.07, 1.0);
    vec4 greenColor = vec4(0.0, 0.5, 0.0, 1.0);

    vec4 finalColor = mix( greenColor,brownColor, colorFactor);

    fragColor = vec3(finalColor.r, finalColor.g, finalColor.b);
    factor = gs_in[0].factor;

    v_uv = uv;
    EmitVertex();
}

void main()
{
    vec3 A = gs_in[0].start;
    vec3 B = gs_in[0].end;

    float radiusA = 1.1*gs_in[0].prevFactor;
    float radiusB = 1.1*gs_in[0].factor;

    vec3 dir = normalize(B - A);

    vec3 arbitrary = vec3(0,1,0);

    if(abs(dot(arbitrary, dir)) > 0.99)
        arbitrary = vec3(1,0,0);

    vec3 right =
        normalize(cross(dir, arbitrary));

    vec3 up =
        normalize(cross(right, dir));

    for(int i = 0; i <= SIDES; i++)
    {   

        float u = float(i) / float(SIDES);

        float angle =
            2.0 * 3.141592 *
            float(i) /
            float(SIDES);

        vec3 offsetA =
            right * cos(angle) * radiusA +
            up    * sin(angle) * radiusA;

        vec3 offsetB =
            right * cos(angle) * radiusB +
            up    * sin(angle) * radiusB;
        float smallerFactor = 0.6;
        float amplificationA = (gs_in[0].prevFactor/leafStartFactor)+leafAmplification;
        float amplificationB = (gs_in[0].factor/leafStartFactor)+leafAmplification;
        vec2 uvA = vec2(u, 0.0);
        vec2 uvB = vec2(u, 1.0);
        if (gs_in[0].prevFactor < leafStartFactor)
        {
            offsetA = right * cos(angle) * radiusA * amplificationA + up * sin(angle) *radiusA / amplificationA;
            uvA = vec2(u/amplificationA, 0.0);
        }
        
        if (gs_in[0].factor < leafStartFactor)
        {
            offsetB = right * cos(angle) * radiusB * amplificationB + up * sin(angle) * radiusB/ amplificationB;
            uvB = vec2(u/amplificationB, 1.0);
        }

        emitVertex(A + offsetA, gs_in[0].prevFactor, vec2(u , 0.0));
        emitVertex(B + offsetB, gs_in[0].factor, vec2(u , 1.0));
    }

    EndPrimitive();
}