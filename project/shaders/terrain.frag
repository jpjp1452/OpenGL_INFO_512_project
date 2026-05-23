#version 400 core

#define MAX_IMPACTS 16

uniform int impactCount;
uniform vec3 impactPos[MAX_IMPACTS];
uniform float impactTime[MAX_IMPACTS];
uniform float time;

in TES_OUT {
    vec3 pos;
    vec2 texCoord;
    vec3 normal;
    float height;
} frag_in;

out vec4 FragColor;
uniform sampler2D textureBrickColor;
uniform sampler2D textureBrickBump;
uniform vec3 lightPos;
uniform float HEIGHT_SCALE;

float computeWave(vec3 worldPos, vec3 center, float t)
{
    float dist = distance(worldPos.xz, center.xz);

    float speed = 25.0;
    float frequency = 20.0;
    float damping = 3.0;

    float wave = sin(dist * frequency - t * speed);
    wave *= exp(-dist * damping);

    return wave;
}

void main()
{
    // Color based on height

    // Height-based biome colors with smooth transitions
    /*
    float h = clamp(frag_in.height, 0.0, 1.0);

    vec3 deepWater    = vec3(0.00, 0.05, 0.25);
    vec3 shallowWater = vec3(0.00, 0.25, 0.55);
    vec3 sand         = vec3(0.76, 0.70, 0.50);
    vec3 grass        = vec3(0.18, 0.45, 0.20);
    vec3 rock         = vec3(0.45, 0.45, 0.45);
    vec3 snow         = vec3(0.95, 0.97, 1.00);

    // Smooth blend factors (start, end, h)
    float w1 = smoothstep(0.08, 0.22, h); // deep -> shallow water
    float w2 = smoothstep(0.22, 0.30, h); // water -> sand
    float w3 = smoothstep(0.30, 0.48, h); // sand -> grass
    float w4 = smoothstep(0.48, 0.72, h); // grass -> rock
    float w5 = smoothstep(0.72, 0.92, h); // rock -> snow

    vec3 color = deepWater;
    color = mix(color, shallowWater, w1);
    color = mix(color, sand,         w2);
    color = mix(color, grass,        w3);
    color = mix(color, rock,         w4);
    color = mix(color, snow,         w5);
    */
    
    // Texture atlas blend: smoothly transition from left half to right half by height.
    vec2 uvBase = frag_in.texCoord * 0.5;
    vec2 uvLow = uvBase;
    vec2 uvHigh = uvBase + vec2(0.5, 0.0);
    float blend = smoothstep(0.20, 0.80, frag_in.height);

    vec3 colorLow = texture(textureBrickColor, uvLow).rgb;
    vec3 colorHigh = texture(textureBrickColor, uvHigh).rgb;
    vec3 color = mix(colorLow, colorHigh, blend);

    // Blend bump gradients from both atlas regions for a smooth normal transition.
    vec2 texel = 1.0 / vec2(textureSize(textureBrickBump, 0));

    float lowL = texture(textureBrickBump, uvLow - vec2(texel.x, 0.0)).r;
    float lowR = texture(textureBrickBump, uvLow + vec2(texel.x, 0.0)).r;
    float lowD = texture(textureBrickBump, uvLow - vec2(0.0, texel.y)).r;
    float lowU = texture(textureBrickBump, uvLow + vec2(0.0, texel.y)).r;

    float highL = texture(textureBrickBump, uvHigh - vec2(texel.x, 0.0)).r;
    float highR = texture(textureBrickBump, uvHigh + vec2(texel.x, 0.0)).r;
    float highD = texture(textureBrickBump, uvHigh - vec2(0.0, texel.y)).r;
    float highU = texture(textureBrickBump, uvHigh + vec2(0.0, texel.y)).r;

    float dXLow = lowR - lowL;
    float dYLow = lowU - lowD;
    float dXHigh = highR - highL;
    float dYHigh = highU - highD;

    float heightScale = 10000.0;
    float dX = mix(dXLow, dXHigh, blend) * heightScale;
    float dY = mix(dYLow, dYHigh, blend) * heightScale;

    vec3 n = normalize(frag_in.normal);
    vec3 t = (abs(n.y) < 0.999) ? normalize(cross(vec3(0.0, 1.0, 0.0), n)) : vec3(1.0, 0.0, 0.0);
    vec3 b = normalize(cross(n, t));
    vec3 bumpedNormal = normalize(n - dX * t - dY * b);

    vec3 toLight = lightPos - frag_in.pos;
    float lightLen = length(toLight);
    vec3 lightDir = (lightLen > 1e-5) ? (toLight / lightLen) : normalize(vec3(0.4, 1.0, 0.2));

    float diff = max(dot(bumpedNormal, lightDir), 0.0);
    float ambient = 0.25;
    vec3 finalColor = color * (ambient + diff);


    float waveSum = 0.0;

    for (int i = 0; i < impactCount; i++)
    {
        float t = time - impactTime[i];
        if (t < 0.0) continue;

        float w = computeWave(frag_in.pos, impactPos[i], t);

        // fade with time
        float lifeFade = exp(-t * 2.5);

        waveSum += w * lifeFade;
    }

    finalColor += waveSum * vec3(1.0, 0.6, 0.2);

    FragColor = vec4(finalColor, 1.0);
}
