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

// Post controls
uniform float uBloomIntensity;
uniform vec2  uSunScreenPos;
uniform float uGodRayIntensity;
uniform float uColorGradeIntensity;
uniform float uVignetteIntensity;
uniform vec2  uWindDir;

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





// ---------- God Rays (radial blur from sun) ----------
vec3 godRays(vec2 uv)
{
    if (uGodRayIntensity < 0.01 || uSunIntensity < 0.01) return vec3(0.0);

    vec2  delta = (uv - uSunScreenPos) / 64.0;
    vec2  sUV   = uv;
    float illum = 0.0;
    float w     = 1.0;

    for (int i = 0; i < 64; i++)
    {
        sUV -= delta;
        float sd = texture(uDepth, clamp(sUV, 0.001, 0.999)).r;
        illum += smoothstep(0.997, 0.9999, sd) * w;
        w *= 0.975;
    }
    illum /= 64.0;

    return vec3(1.0, 0.94, 0.82) * uSunIntensity * illum * uGodRayIntensity * 2.5;
}

void main()
{
    vec3  sceneCol = texture(uScene, vUV).rgb;
    float rawD     = texture(uDepth, vUV).r;
    float linearD  = linearizeDepth(rawD);
    vec3  rayW     = worldRayDir(vUV);

    // ---- Bloom composite ----
    sceneCol += texture(uBloom, vUV).rgb * uBloomIntensity;

    // ---- Sun disc + corona ----
    if (uSunIntensity > 0.001)
    {
        float skyMask = smoothstep(0.997, 0.9999, rawD)
                      * smoothstep(uFar * 0.35, uFar * 0.92, linearD);
        vec3  sdir = normalize(uSunDir);
        float mu   = max(dot(rayW, sdir), 0.0);
        float disc = smoothstep(cos(radians(7.5)), cos(radians(2.8)), mu) * uSunIntensity * skyMask;
        float corona = pow(mu, 96.0) * uSunIntensity * skyMask;
        sceneCol += vec3(1.0, 0.94, 0.82) * disc * 3.2;
        sceneCol += vec3(1.0, 0.88, 0.65) * corona * 1.4;
    }

    // ---- God Rays ----
    sceneCol += godRays(vUV);

    // ---- Wetness Desaturation ----
    if (uRainIntensity > 0.001)
    {
        // Wet desaturation and tinting
        sceneCol *= 1.0 - uRainIntensity * 0.12;
        sceneCol  = mix(sceneCol, sceneCol * vec3(0.95, 0.98, 1.04), uRainIntensity * 0.2);
    }

    // ---- Color Grading (urban rainy mood) ----
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

    // ---- Film Grain (subtle) ----
    sceneCol += (hash12(vUV * uResolution + vec2(uTime * 100.0, 0.0)) - 0.5) * 0.022;

    FragColor = vec4(max(sceneCol, vec3(0.0)), 1.0);
}
