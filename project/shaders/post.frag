#version 330 core
in vec2 vUV;

uniform sampler2D uScene;
uniform sampler2D uDepth;
uniform sampler2D uBloom;
uniform vec2  uResolution;
uniform float uTime;
uniform float uRainIntensity;

uniform mat4  uInvProj;
uniform mat4  uInvView;
uniform float uNear;
uniform float uFar;

uniform vec3  uSunDir;
uniform float uSunIntensity;
uniform vec3  uSkyColor;
uniform vec3  uGroundColor;
uniform float uDayFactor;

// Post controls
uniform float uBloomIntensity;
uniform vec2  uSunScreenPos;
uniform float uGodRayIntensity;
uniform float uColorGradeIntensity;
uniform float uVignetteIntensity;
uniform vec2  uWindDir;
uniform float uFxaaIntensity;
uniform float uChromaticAberration;

// SSAO + SSR
uniform sampler2D uSSAOTex;
uniform sampler2D uNormalTex;
uniform mat4  uProjection;
uniform float uSSAOIntensity;
uniform float uSSRIntensity;

out vec4 FragColor;

// ---------- Helpers ----------
float hash12(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float linearizeDepth(float raw)
{
    float zNdc = raw * 2.0 - 1.0;
    return (2.0 * uNear * uFar) / (uFar + uNear - zNdc * (uFar - uNear));
}

vec3 worldRayDir(vec2 uv)
{
    vec2 ndc  = uv * 2.0 - 1.0;
    vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 eye  = uInvProj * clip;
    eye = vec4(eye.xy, -1.0, 0.0);
    return normalize(mat3(uInvView) * normalize(eye.xyz));
}

// ---------- Procedural Sky ----------
vec3 proceduralSky(vec3 rd, vec3 sunDir, float dayF)
{
    float sunElev = sunDir.y;
    float upDot = rd.y;

    // --- Sky gradient ---
    vec3 zenithDay   = vec3(0.12, 0.30, 0.75); // Deeper, more saturated blue
    vec3 horizonDay  = vec3(0.55, 0.75, 0.95);
    vec3 zenithNight = vec3(0.01, 0.02, 0.05);
    vec3 horizonNight= vec3(0.06, 0.08, 0.15); // Slight city light glow at night horizon

    vec3 zenith  = mix(zenithNight, zenithDay, dayF);
    vec3 horizon = mix(horizonNight, horizonDay, dayF);
    vec3 sky = mix(horizon, zenith, clamp(upDot, 0.0, 1.0));

    // Below horizon — darken toward ground
    if (upDot < 0.0) {
        vec3 ground = mix(horizonNight * 0.5, uGroundColor * 0.6, dayF);
        sky = mix(sky, ground, clamp(-upDot * 3.0, 0.0, 1.0));
    }

    // --- Sunset/sunrise horizon glow ---
    float horizonGlow = exp(-abs(upDot) * 5.0);
    float sunCloseness = max(dot(rd, sunDir), 0.0);
    vec3 sunsetColor = vec3(1.0, 0.45, 0.15);
    vec3 dawnColor   = vec3(1.0, 0.65, 0.35);
    vec3 glowCol = mix(sunsetColor, dawnColor, clamp(sunElev * 3.0 + 0.5, 0.0, 1.0));
    float twilightMask = smoothstep(0.85, 0.0, dayF) * smoothstep(-0.15, 0.15, sunElev);
    sky += glowCol * horizonGlow * sunCloseness * twilightMask * 1.8;
    sky += glowCol * horizonGlow * twilightMask * 0.25;

    // --- Sun disc + corona ---
    float mu = max(dot(rd, sunDir), 0.0);
    float disc = smoothstep(cos(radians(1.8)), cos(radians(0.5)), mu);
    float corona = pow(mu, 128.0);
    float softGlow = pow(mu, 8.0);
    vec3 sunCol = vec3(1.0, 0.95, 0.85);
    float sunVis = clamp(sunElev + 0.05, 0.0, 1.0);
    sky += sunCol * disc * 15.0 * sunVis;
    sky += sunCol * corona * 3.0 * sunVis;
    sky += vec3(1.0, 0.85, 0.6) * softGlow * 0.15 * clamp(sunElev, 0.0, 1.0);

    // --- Stars at night ---
    // (Removed because they looked like flashing white artifacts)
    // --- Clouds ---
    if (upDot > 0.0) {
        float cloudHeight = 0.3;
        vec2 cloudUV = rd.xz / (rd.y + cloudHeight) * 8.0;
        cloudUV += uWindDir * uTime * 0.005;
        float c1 = hash12(floor(cloudUV * 1.0));
        float c2 = hash12(floor(cloudUV * 2.0 + 7.3));
        float cloud = smoothstep(0.55, 0.7, c1 * 0.6 + c2 * 0.4);
        cloud *= smoothstep(0.0, 0.15, upDot);
        vec3 cloudCol = mix(vec3(0.04), vec3(0.9, 0.92, 0.95), dayF);
        cloudCol = mix(cloudCol, glowCol * 1.3, twilightMask * cloud * 0.5);
        sky = mix(sky, cloudCol, cloud * 0.35);
        // Heavy overcast when raining
        if (uRainIntensity > 0.1) {
            float rainCloud = smoothstep(0.35, 0.5, c1 * 0.5 + c2 * 0.5) * uRainIntensity;
            sky = mix(sky, vec3(0.25, 0.27, 0.3) * dayF, rainCloud * 0.6);
        }
    }

    return sky;
}

// ---------- God Rays ----------
vec3 godRays(vec2 uv)
{
    if (uGodRayIntensity < 0.01 || uSunIntensity < 0.01) return vec3(0.0);

    vec2  delta = (uv - uSunScreenPos) / 32.0;
    vec2  sUV   = uv;
    float illum = 0.0;
    float w     = 1.0;

    for (int i = 0; i < 32; i++)
    {
        sUV -= delta;
        float sd = texture(uDepth, clamp(sUV, 0.001, 0.999)).r;
        illum += smoothstep(0.997, 0.9999, sd) * w;
        w *= 0.95;
    }
    illum /= 32.0;

    return vec3(1.0, 0.94, 0.82) * uSunIntensity * illum * uGodRayIntensity * 2.5;
}

// ---------- Simplified FXAA ----------
vec3 applyFXAA(vec2 uv)
{
    vec2 texel = 1.0 / uResolution;
    vec3 col   = texture(uScene, uv).rgb;
    float lumaM  = dot(col, vec3(0.299, 0.587, 0.114));
    float lumaN  = dot(texture(uScene, uv + vec2(0, texel.y)).rgb, vec3(0.299, 0.587, 0.114));
    float lumaS  = dot(texture(uScene, uv - vec2(0, texel.y)).rgb, vec3(0.299, 0.587, 0.114));
    float lumaE  = dot(texture(uScene, uv + vec2(texel.x, 0)).rgb, vec3(0.299, 0.587, 0.114));
    float lumaW  = dot(texture(uScene, uv - vec2(texel.x, 0)).rgb, vec3(0.299, 0.587, 0.114));

    float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    float contrast = lumaMax - lumaMin;

    if (contrast < 0.03) return col;

    float edgeFilter = abs(lumaN + lumaS + lumaE + lumaW - 4.0 * lumaM);
    edgeFilter = clamp(edgeFilter / contrast, 0.0, 1.0);
    float blend = smoothstep(0.0, 1.0, edgeFilter) * 0.75;

    bool isHorizontal = abs(lumaN + lumaS - 2.0 * lumaM) >= abs(lumaE + lumaW - 2.0 * lumaM);
    vec2 dir = isHorizontal ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);

    vec3 col1 = texture(uScene, uv - dir).rgb;
    vec3 col2 = texture(uScene, uv + dir).rgb;
    return mix(col, (col1 + col2) * 0.5, blend);
}

void main()
{
    // ---- FXAA ----
    vec3  sceneCol = (uFxaaIntensity > 0.01) ? applyFXAA(vUV) : texture(uScene, vUV).rgb;

    // ---- Chromatic Aberration ----
    if (uChromaticAberration > 0.001)
    {
        vec2 center = vUV - 0.5;
        float dist2 = dot(center, center);
        float caStrength = uChromaticAberration * dist2 * 2.0;
        vec2 caOffset = center * caStrength;
        sceneCol.r = texture(uScene, vUV + caOffset).r;
        sceneCol.b = texture(uScene, vUV - caOffset).b;
    }

    float rawD     = texture(uDepth, vUV).r;
    float linearD  = linearizeDepth(rawD);
    vec3  rayW     = worldRayDir(vUV);

    // ---- Procedural sky for far pixels ----
    // Fix: only draw sky where depth is exactly 1.0 (far plane / cleared depth), 
    // to prevent buildings from becoming transparent.
    float skyMask = step(0.99999, rawD);
    if (skyMask > 0.5) {
        vec3 skyCol = proceduralSky(rayW, normalize(uSunDir), uDayFactor);
        sceneCol = mix(sceneCol, skyCol, skyMask);
    }

    // ---- SSAO composite ----
    if (uSSAOIntensity > 0.01 && skyMask < 0.5) {
        float ao = texture(uSSAOTex, vUV).r;
        ao = mix(1.0, ao, uSSAOIntensity);
        sceneCol *= ao;
    }

    // ---- SSR (Screen-Space Reflections) — wet ground only ----
    if (uSSRIntensity > 0.01 && uRainIntensity > 0.1 && skyMask < 0.5) {
        vec3 viewNormal = normalize(texture(uNormalTex, vUV).rgb * 2.0 - 1.0);
        // Only reflect on roughly upward-facing surfaces (ground/road)
        if (viewNormal.y < -0.5) { // In view space, ground normals point -Y (toward camera top)
            // Reconstruct view-space position
            vec2 ndc = vUV * 2.0 - 1.0;
            vec4 clipPos = vec4(ndc, rawD * 2.0 - 1.0, 1.0);
            vec4 viewPos4 = uInvProj * clipPos;
            vec3 viewPos = viewPos4.xyz / viewPos4.w;

            vec3 viewDir = normalize(viewPos);
            vec3 reflDir = reflect(viewDir, viewNormal);

            // Ray-march in screen space
            vec3 ssrColor = vec3(0.0);
            bool hit = false;
            float stepSize = 0.06;
            vec3 marchPos = viewPos;

            for (int i = 0; i < 16; i++) {
                marchPos += reflDir * stepSize;
                stepSize *= 1.15; // Accelerate steps

                // Project to screen
                vec4 projPos = uProjection * vec4(marchPos, 1.0);
                projPos.xyz /= projPos.w;
                vec2 sampleUV = projPos.xy * 0.5 + 0.5;

                if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) break;

                float sampledDepth = texture(uDepth, sampleUV).r;
                vec4 sampledClip = vec4(sampleUV * 2.0 - 1.0, sampledDepth * 2.0 - 1.0, 1.0);
                vec4 sampledView4 = uInvProj * sampledClip;
                float sampledViewZ = sampledView4.z / sampledView4.w;

                if (marchPos.z < sampledViewZ && marchPos.z > sampledViewZ - 0.5) {
                    ssrColor = texture(uScene, sampleUV).rgb;
                    // Fade at screen edges
                    float edgeFade = 1.0 - smoothstep(0.0, 0.1, max(max(abs(sampleUV.x - 0.5) - 0.4, 0.0), max(abs(sampleUV.y - 0.5) - 0.4, 0.0)) * 10.0);
                    ssrColor *= edgeFade;
                    hit = true;
                    break;
                }
            }

            if (hit) {
                float reflStrength = uSSRIntensity * uRainIntensity * 0.5;
                sceneCol = mix(sceneCol, ssrColor, reflStrength);
            }
        }
    }

    // ---- Bloom composite ----
    sceneCol += texture(uBloom, vUV).rgb * uBloomIntensity;

    // ---- God Rays ----
    sceneCol += godRays(vUV);

    // ---- Wetness Desaturation ----
    if (uRainIntensity > 0.001)
    {
        sceneCol *= 1.0 - uRainIntensity * 0.12;
        sceneCol  = mix(sceneCol, sceneCol * vec3(0.95, 0.98, 1.04), uRainIntensity * 0.2);
    }

    // ---- Color Grading ----
    if (uColorGradeIntensity > 0.01)
    {
        float luma = dot(sceneCol, vec3(0.2126, 0.7152, 0.0722));
        vec3  shadowTint = vec3(0.85, 0.90, 1.12);
        vec3  highTint   = vec3(1.08, 1.02, 0.94);
        vec3  graded = sceneCol * mix(shadowTint, highTint, smoothstep(0.0, 0.7, luma));
        sceneCol = mix(sceneCol, graded, uColorGradeIntensity);
        sceneCol = mix(vec3(luma), sceneCol, mix(1.0, 0.88, uColorGradeIntensity));
    }

    // ---- Vignette ----
    if (uVignetteIntensity > 0.01)
    {
        vec2  ctr = vUV - 0.5;
        float vig = smoothstep(0.0, 1.0, 1.0 - dot(ctr, ctr) * uVignetteIntensity * 2.8);
        sceneCol *= vig;
    }

    // ---- Film Grain ----
    sceneCol += (hash12(vUV * uResolution + vec2(uTime * 100.0, 0.0)) - 0.5) * 0.022;

    FragColor = vec4(max(sceneCol, vec3(0.0)), 1.0);
}
