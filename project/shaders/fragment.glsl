#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vFragPosLightSpace;

uniform sampler2D uShadowMap;

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

// Car headlights (max 24 = 12 cars × 2)
#define MAX_HEADLIGHTS 24
uniform int   uHeadlightCount;
uniform vec3  uHeadlightPos[MAX_HEADLIGHTS];
uniform vec3  uHeadlightDir[MAX_HEADLIGHTS];
uniform vec3  uHeadlightColor;
uniform float uHeadlightIntensity;

// Tail lights (red, max 24)
#define MAX_TAILLIGHTS 24
uniform int   uTailLightCount;
uniform vec3  uTailLightPos[MAX_TAILLIGHTS];
uniform float uTailLightBrake[MAX_TAILLIGHTS];
uniform float uTailLightIntensity;

// Street lamps (max 32)
#define MAX_STREETLAMPS 32
uniform int   uStreetLampCount;
uniform vec3  uStreetLampPos[MAX_STREETLAMPS];
uniform vec3  uStreetLampColor;
uniform float uStreetLampIntensity;

out vec4 FragColor;

// --------------- ACES Filmic ---------------
vec3 acesToneMap(vec3 x)
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    return shadow * smoothstep(0.0, 0.4, uDayFactor); // Softer shadows at dawn/dusk
}

float hash2d(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// ---------- Rain Splash ----------
float rainSplash(vec2 px, float t, float inten)
{
    if (inten < 0.1) return 0.0;

    float scale = 0.3;
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

// ---------- Headlight contribution ----------
vec3 calcHeadlights(vec3 fragPos, vec3 N, vec3 albedo)
{
    if (uHeadlightCount <= 0 || uHeadlightIntensity < 0.01) return vec3(0.0);

    vec3 totalLight = vec3(0.0);
    for (int i = 0; i < MAX_HEADLIGHTS; i++)
    {
        if (i >= uHeadlightCount) break;

        vec3 toFrag = fragPos - uHeadlightPos[i];
        float dist = length(toFrag);
        float atten = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        if (atten < 0.005) continue;

        vec3 lightToFrag = normalize(toFrag);
        float spotAngle = dot(lightToFrag, uHeadlightDir[i]);
        float spotCone = smoothstep(cos(radians(35.0)), cos(radians(15.0)), spotAngle);
        if (spotCone < 0.001) continue;

        float NdotL = max(dot(N, -lightToFrag), 0.0);
        vec3 contrib = albedo * uHeadlightColor * NdotL * atten * spotCone * uHeadlightIntensity;

        vec3 V = normalize(uViewPos - fragPos);
        vec3 H = normalize(-lightToFrag + V);
        float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.3;
        contrib += uHeadlightColor * spec * atten * spotCone * uHeadlightIntensity;

        totalLight += contrib;
    }
    return totalLight;
}

// ---------- Tail light contribution ----------
vec3 calcTailLights(vec3 fragPos, vec3 N, vec3 albedo)
{
    if (uTailLightCount <= 0 || uTailLightIntensity < 0.01) return vec3(0.0);

    vec3 totalLight = vec3(0.0);
    vec3 tailColor = vec3(1.0, 0.08, 0.05); // deep red

    for (int i = 0; i < MAX_TAILLIGHTS; i++)
    {
        if (i >= uTailLightCount) break;

        vec3 toFrag = fragPos - uTailLightPos[i];
        float dist = length(toFrag);
        float atten = 1.0 / (1.0 + 0.18 * dist + 0.08 * dist * dist);
        if (atten < 0.005) continue;

        float NdotL = max(dot(N, -normalize(toFrag)), 0.0);
        float brake = uTailLightBrake[i];
        totalLight += albedo * tailColor * NdotL * atten * uTailLightIntensity * brake;
    }
    return totalLight;
}

// ---------- Street lamp contribution ----------
vec3 calcStreetLamps(vec3 fragPos, vec3 N, vec3 albedo)
{
    if (uStreetLampCount <= 0 || uStreetLampIntensity < 0.01) return vec3(0.0);

    vec3 totalLight = vec3(0.0);
    for (int i = 0; i < MAX_STREETLAMPS; i++)
    {
        if (i >= uStreetLampCount) break;

        vec3 toFrag = fragPos - uStreetLampPos[i];
        float dist = length(toFrag);
        // Street lamps: larger range than headlights
        float atten = 1.0 / (1.0 + 0.06 * dist + 0.015 * dist * dist);
        if (atten < 0.003) continue;

        float NdotL = max(dot(N, -normalize(toFrag)), 0.0);
        vec3 contrib = albedo * uStreetLampColor * NdotL * atten * uStreetLampIntensity;

        // Specular
        vec3 V = normalize(uViewPos - fragPos);
        vec3 H = normalize(-normalize(toFrag) + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.15;
        contrib += uStreetLampColor * spec * atten * uStreetLampIntensity;

        totalLight += contrib;
    }
    return totalLight;
}

// ---------- Road markings ----------
vec3 roadMarkings(vec3 worldPos, vec3 N, vec3 baseColor)
{
    if (N.y < 0.7) return baseColor; // Only on ground
    float wx = worldPos.x;
    float wz = worldPos.z;

    vec3 white = vec3(0.85, 0.85, 0.82);
    float markMix = 0.0;

    // --- Center dashed line (NS road: x ≈ 0, along Z) ---
    if (abs(wx) < 5.5) {
        // Center line
        float centerLine = 1.0 - smoothstep(0.0, 0.12, abs(wx));
        // Dashed: 3 units on, 3 units off
        float dash = step(0.5, fract(wz / 6.0));
        markMix = max(markMix, centerLine * dash);

        // Lane edge lines (solid, at x = ±4.5)
        float leftEdge = 1.0 - smoothstep(0.0, 0.1, abs(wx - 4.8));
        float rightEdge = 1.0 - smoothstep(0.0, 0.1, abs(wx + 4.8));
        markMix = max(markMix, leftEdge);
        markMix = max(markMix, rightEdge);
    }

    // --- Center dashed line (EW road: z ≈ 0, along X) ---
    if (abs(wz) < 5.5) {
        float centerLine = 1.0 - smoothstep(0.0, 0.12, abs(wz));
        float dash = step(0.5, fract(wx / 6.0));
        markMix = max(markMix, centerLine * dash);

        float topEdge = 1.0 - smoothstep(0.0, 0.1, abs(wz - 4.8));
        float botEdge = 1.0 - smoothstep(0.0, 0.1, abs(wz + 4.8));
        markMix = max(markMix, topEdge);
        markMix = max(markMix, botEdge);
    }

    // --- Crosswalk stripes at intersection ---
    // NS crosswalk (at z = ±6, crossing X road)
    for (int s = -1; s <= 1; s += 2) {
        float cz = float(s) * 6.0;
        if (abs(wz - cz) < 1.5 && abs(wx) < 5.0) {
            float stripe = step(0.5, fract(wx / 1.2));
            markMix = max(markMix, stripe * 0.8);
        }
    }
    // EW crosswalk (at x = ±6, crossing Z road)
    for (int s = -1; s <= 1; s += 2) {
        float cx = float(s) * 6.0;
        if (abs(wx - cx) < 1.5 && abs(wz) < 5.0) {
            float stripe = step(0.5, fract(wz / 1.2));
            markMix = max(markMix, stripe * 0.8);
        }
    }

    return mix(baseColor, white, markMix * 0.85);
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

    // ---------- Road markings ----------
    albedo = roadMarkings(vWorldPos, N, albedo);

    // ---------- Wet surface & Puddles ----------
    bool  isGround = N.y > 0.7;
    float puddleMask = 0.0;
    if (isGround && uWetness > 0.05) {
        float n1 = hash2d(floor(vWorldPos.xz * 0.3));
        float n2 = hash2d(floor(vWorldPos.xz * 1.5 + vec2(15.2, 88.1)));
        puddleMask = smoothstep(0.45, 0.65, (n1 * 0.6 + n2 * 0.4));
        puddleMask *= clamp(uWetness * 2.5, 0.0, 1.0);
    }

    float wetFactor = uWetness * (isGround ? 1.0 : 0.35);
    albedo *= mix(1.0, 0.52, wetFactor);

    if (isGround && uWetness > 0.1) {
        float splash = rainSplash(vWorldPos.xz, uTime, uWetness);
        albedo += vec3(0.5, 0.6, 0.7) * splash * 0.5;
    }

    // ---------- Shadow ----------
    float shadow = ShadowCalculation(vFragPosLightSpace, N, L);

    // ---------- Hemisphere ambient ----------
    float hemi    = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3  ambientTerm = albedo * mix(uGroundColor, uSkyColor, hemi);

    // ---------- Wrap diffuse ----------
    const float wrap = 0.12;
    float NdotL = max(dot(N, L), 0.0);
    float diff  = clamp((NdotL + wrap) / (1.0 + wrap), 0.0, 1.0);
    vec3  diffuseTerm = albedo * uSunColor * diff * (1.0 - shadow);

    // ---------- Specular & Puddle Reflection ----------
    float shininess    = mix(48.0, 256.0, max(wetFactor, puddleMask));
    float specStrength = mix(0.22, 1.2, puddleMask);
    if (puddleMask < 0.001) specStrength = mix(0.22, 0.65, wetFactor);

    float specMask     = pow(max(dot(N, H), 0.0), shininess);
    vec3  specularTerm = mix(vec3(1.0), uSunColor * 0.35, 0.5) * specMask * specStrength * (1.0 - shadow);

    if (puddleMask > 0.01) {
        vec3 R = reflect(-V, N);
        float upFactor = clamp(R.y, 0.0, 1.0);
        vec3 envReflect = mix(uGroundColor, uSkyColor, upFactor);
        specularTerm += envReflect * puddleMask * 0.4;
    }

    // ---------- Fresnel wet reflection ----------
    float fresnel = 0.0;
    if (max(wetFactor, puddleMask) > 0.01 && isGround)
    {
        float NdotV = max(dot(N, V), 0.0);
        fresnel = pow(1.0 - NdotV, 4.0) * mix(0.55, 0.85, puddleMask);
    }

    vec3 color = ambientTerm + diffuseTerm + specularTerm;
    color = mix(color, uSkyColor * 0.45 + uSunColor * 0.12, fresnel);

    // ---------- Artificial lighting ----------
    color += calcHeadlights(vWorldPos, N, albedo);
    color += calcTailLights(vWorldPos, N, albedo);
    color += calcStreetLamps(vWorldPos, N, albedo);

    // ---------- Emissive windows ----------
    if (uWindowEmissive > 0.01 && uDayFactor < 0.85)
    {
        bool isWall = abs(N.y) < 0.3;
        if (isWall)
        {
            float gx = step(0.42, fract(vTexCoord.x * 4.2));
            float gy = step(0.32, fract(vTexCoord.y * 6.8));
            float wMask = gx * gy;
            vec2  cellId = floor(vec2(vTexCoord.x * 4.2, vTexCoord.y * 6.8));
            wMask *= step(0.35, hash2d(cellId));
            float emStr = wMask * uWindowEmissive * (1.0 - uDayFactor);
            color += vec3(1.0, 0.92, 0.72) * emStr;
        }
    }

    // ---------- Neon billboard signs ----------
    if (uDayFactor < 0.7)
    {
        bool isWall = abs(N.y) < 0.3;
        if (isWall && vWorldPos.y > 3.0 && vWorldPos.y < 18.0)
        {
            float nightStr = 1.0 - smoothstep(0.0, 0.7, uDayFactor);

            // Divide wall into large sign zones
            vec2 signZone = floor(vec2(vTexCoord.x * 1.5, vTexCoord.y * 2.0));
            float zoneHash = hash2d(signZone * 31.7 + vec2(13.1, 7.3));

            // Only ~30% of zones have neon signs
            if (zoneHash > 0.7)
            {
                // Pick neon color based on zone
                vec3 neonCol;
                float colorSel = fract(zoneHash * 7.13);
                if (colorSel < 0.25)      neonCol = vec3(0.1, 0.85, 1.0);   // cyan
                else if (colorSel < 0.5)  neonCol = vec3(1.0, 0.15, 0.55);  // magenta/pink
                else if (colorSel < 0.75) neonCol = vec3(1.0, 0.55, 0.08);  // orange
                else                      neonCol = vec3(0.2, 1.0, 0.3);    // green

                // Sign pattern: horizontal bars
                float barFreq = mix(6.0, 12.0, fract(zoneHash * 3.7));
                float barPhase = fract(vTexCoord.y * barFreq);
                float bar = smoothstep(0.3, 0.35, barPhase) * (1.0 - smoothstep(0.55, 0.6, barPhase));

                // Vertical accent lines
                float lineFreq = mix(3.0, 8.0, fract(zoneHash * 5.1));
                float linePhase = fract(vTexCoord.x * lineFreq);
                float vLine = smoothstep(0.42, 0.45, linePhase) * (1.0 - smoothstep(0.52, 0.55, linePhase));

                float signShape = max(bar * 0.8, vLine * 0.5);

                // Flicker animation (some signs flicker more than others)
                float flickerSpeed = mix(2.0, 8.0, fract(zoneHash * 11.3));
                float flickerAmt = fract(zoneHash * 2.7) * 0.3;
                float flicker = 1.0 - flickerAmt + flickerAmt * (0.5 + 0.5 * sin(uTime * flickerSpeed + zoneHash * 50.0));

                // Occasional "broken" sign effect
                float broken = step(0.92, fract(zoneHash * 19.1));
                if (broken > 0.5) {
                    flicker *= step(0.3, fract(uTime * 1.5 + zoneHash * 100.0));
                }

                float neonStr = signShape * nightStr * flicker * 3.5;
                color += neonCol * neonStr;

                // Glow bleed (soft halo around neon)
                float glow = smoothstep(0.6, 0.0, abs(barPhase - 0.45)) * 0.15;
                glow += smoothstep(0.6, 0.0, abs(linePhase - 0.48)) * 0.1;
                color += neonCol * glow * nightStr * flicker;
            }
        }
    }

    // ---------- Height-based fog ----------
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
