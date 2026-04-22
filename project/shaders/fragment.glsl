#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3  uLightDir;
uniform vec3  uSkyColor;
uniform vec3  uGroundColor;
uniform vec3  uSunColor;
uniform vec3  uViewPos;
uniform float uExposure;

// Fog
uniform vec3  uFogColor;
uniform float uFogDensity;
uniform float uFogBaseHeight;
uniform float uFogHeightFalloff;

// Wet surface
uniform float uWetness;

// Emissive windows
uniform float uWindowEmissive;
uniform float uDayFactor;
uniform float uTime;

// Material
uniform vec4      uBaseColorFactor;
uniform sampler2D uDiffuseTex;
uniform bool      uUseTexture;

out vec4 FragColor;

// --------------- ACES Filmic ---------------
vec3 acesToneMap(vec3 x)
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float hash2d(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// ---------- Rain Splash (expanding rings on ground) ----------
float rainSplash(vec2 px, float t, float inten)
{
    if (inten < 0.1) return 0.0;

    float scale = 0.3; // 30cm grid
    vec2  cell = floor(px / scale);
    vec2  fp   = fract(px / scale) - 0.5;
    float splash = 0.0;

    for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++)
    {
        vec2  nb    = cell + vec2(float(dx), float(dy));
        float rnd   = hash2d(nb * 0.73 + vec2(17.3, 31.7));
        float phase = fract(t * mix(1.8, 4.0, rnd) + rnd * 6.28);

        vec2  off   = vec2(hash2d(nb + 0.1), hash2d(nb + 0.2)) - 0.5;
        float dist  = length(fp - vec2(float(dx), float(dy)) - off * 0.5);

        float ring  = 1.0 - smoothstep(0.0, 0.06, abs(dist - phase * 0.4));
        ring *= (1.0 - phase); 
        ring *= step(0.4 - inten * 0.3, rnd); 
        splash += ring;
    }
    return clamp(splash * inten * 2.5, 0.0, 1.0);
}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    // ---------- Albedo ----------
    vec3 albedo = uBaseColorFactor.rgb;
    if (uUseTexture)
        albedo *= texture(uDiffuseTex, vTexCoord).rgb;

    // ---------- Wet surface ----------
    bool  isGround = N.y > 0.7;
    float wetFactor = uWetness * (isGround ? 1.0 : 0.35);
    albedo *= mix(1.0, 0.52, wetFactor);

    if (isGround && uWetness > 0.1) {
        float splash = rainSplash(vWorldPos.xz, uTime, uWetness);
        albedo += vec3(0.5, 0.6, 0.7) * splash * 0.5; 
    }

    // ---------- Hemisphere ambient ----------
    float hemi    = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3  ambientTerm = albedo * mix(uGroundColor, uSkyColor, hemi);

    // ---------- Wrap diffuse ----------
    const float wrap = 0.12;
    float NdotL = max(dot(N, L), 0.0);
    float diff  = clamp((NdotL + wrap) / (1.0 + wrap), 0.0, 1.0);
    vec3  diffuseTerm = albedo * uSunColor * diff;

    // ---------- Specular (boosted when wet) ----------
    float shininess    = mix(48.0, 196.0, wetFactor);
    float specStrength = mix(0.22, 0.65, wetFactor);
    float specMask     = pow(max(dot(N, H), 0.0), shininess);
    vec3  specularTerm = mix(vec3(1.0), uSunColor * 0.35, 0.5) * specMask * specStrength;

    // ---------- Fresnel wet reflection ----------
    float fresnel = 0.0;
    if (wetFactor > 0.01 && isGround)
    {
        float NdotV = max(dot(N, V), 0.0);
        fresnel = pow(1.0 - NdotV, 4.0) * wetFactor * 0.55;
    }

    vec3 color = ambientTerm + diffuseTerm + specularTerm;
    color = mix(color, uSkyColor * 0.45 + uSunColor * 0.12, fresnel);

    // ---------- Emissive windows (heuristic) ----------
    if (uWindowEmissive > 0.01 && uDayFactor < 0.85)
    {
        bool isWall = abs(N.y) < 0.3;
        if (isWall)
        {
            float gx = step(0.42, fract(vTexCoord.x * 4.2));
            float gy = step(0.32, fract(vTexCoord.y * 6.8));
            float wMask = gx * gy;
            // Random on/off per window cell
            vec2  cellId = floor(vec2(vTexCoord.x * 4.2, vTexCoord.y * 6.8));
            wMask *= step(0.35, hash2d(cellId));
            float emStr = wMask * uWindowEmissive * (1.0 - uDayFactor);
            color += vec3(1.0, 0.92, 0.72) * emStr;
        }
    }

    // ---------- Height-based fog + sun in-scattering ----------
    if (uFogDensity > 1e-6)
    {
        float dist    = length(uViewPos - vWorldPos);
        float fogDist = 1.0 - exp(-uFogDensity * dist * 0.045);

        float hAbove  = max(vWorldPos.y - uFogBaseHeight, 0.0);
        float fogH    = exp(-hAbove * uFogHeightFalloff);
        float fogAmt  = clamp(fogDist * fogH, 0.0, 1.0);

        float scatter = pow(max(dot(V, normalize(-uLightDir)), 0.0), 8.0);
        vec3  fogCol  = mix(uFogColor, uSunColor * 0.55, scatter * 0.4);
        color = mix(color, fogCol, fogAmt);
    }

    // ---------- Exposure + ACES ----------
    color *= uExposure;
    color  = acesToneMap(color);

    FragColor = vec4(color, uBaseColorFactor.a);
}
