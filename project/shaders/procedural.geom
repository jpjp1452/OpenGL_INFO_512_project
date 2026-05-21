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

void emitVertex(vec3 p, float colorFactor)
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
        float amplicatationA = (gs_in[0].prevFactor/smallerFactor)+4.0;
        float amplicatationB = (gs_in[0].factor/smallerFactor)+4.0;
        
        if (gs_in[0].prevFactor < smallerFactor)
        {
            //offsetA.x *= (gs_in[0].prevFactor/smallerFactor)+4.0;
            //offsetA.y *= (gs_in[0].prevFactor/smallerFactor)+4.0;
            //offsetA.z *= 1.1-(gs_in[0].prevFactor/smallerFactor);
            offsetA = right * cos(angle) * radiusA * amplicatationA + up * sin(angle) *radiusA / amplicatationA;

            
        }
        
        if (gs_in[0].factor < smallerFactor)
        {
            //offsetB.x *= (gs_in[0].factor/smallerFactor)+4.0;
            //offsetB.y *= (gs_in[0].factor/smallerFactor)+4.0;
            //offsetB.z *= 1.1-(gs_in[0].factor/smallerFactor);
            offsetB = right * cos(angle) * radiusB * amplicatationB + up * sin(angle) * radiusB/ amplicatationB;
        }

        emitVertex(A + offsetA, gs_in[0].prevFactor);
        emitVertex(B + offsetB, gs_in[0].factor);
    }

    EndPrimitive();
}