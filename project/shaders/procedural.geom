#version 330 core

layout(points) in;

layout(triangle_strip, max_vertices = 64) out;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

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

void emitVertex(vec3 p)
{
    gl_Position =
        P *
        V *
        M *
        vec4(p, 1.0);

    fragColor = vec3(0.4, 0.2, 0.1);
    factor = gs_in[0].factor;

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
        if (factor < smallerFactor)
        {
            offsetA.x *= (factor/smallerFactor)+4.0;
            offsetB.x *= (factor/smallerFactor)+4.0;
            offsetA.y *= (factor/smallerFactor)+4.0;
            offsetB.y *= (factor/smallerFactor)+4.0;
            offsetA.z *= 1.1-(factor/smallerFactor);
            offsetB.z *= 1.1-(factor/smallerFactor);
        }

        emitVertex(A + offsetA);
        emitVertex(B + offsetB);
    }

    EndPrimitive();
}