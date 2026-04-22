#version 330 core

in vec2 vUV;

uniform sampler2D uScene;
uniform vec2 uResolution;
uniform float uTime;
uniform float uRainIntensity;

out vec4 FragColor;

float hash12(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Mưa dạng vệt mảnh trong không gian màn hình (hướng xiên nhẹ)
float rainStreak(vec2 fragPx, float t, float intensity)
{
    if (intensity <= 0.001)
        return 0.0;

    float scale = mix(90.0, 165.0, intensity);
    vec2 uv = fragPx / uResolution.y;
    float tilt = 0.42;
    uv.x += uv.y * tilt + t * 0.22 * intensity;

    vec2 grid = uv * vec2(scale * 1.15, scale * 1.65);
    vec2 cell = floor(grid);
    float id = hash12(cell);
    float thresh = mix(0.78, 0.52, intensity);
    if (id > thresh)
        return 0.0;

    float scroll = t * (14.0 + 10.0 * intensity) + id * 17.0;
    float fy = fract(grid.y + scroll);
    float streak = smoothstep(0.0, 0.07, fy) * smoothstep(0.55, 0.18, fy);
    streak *= smoothstep(0.25, 1.0, intensity);
    return streak * (0.14 + 0.38 * intensity);
}

void main()
{
    vec3 sceneCol = texture(uScene, vUV).rgb;
    vec2 px = vUV * uResolution;
    float r = rainStreak(px, uTime, uRainIntensity);
    vec3 rainTint = vec3(0.74, 0.82, 0.95);
    vec3 outCol = sceneCol + rainTint * r;

    // Hơi làm tối nhẹ khi mưa to (ẩm ướt)
    outCol *= 1.0 - uRainIntensity * 0.12;

    FragColor = vec4(outCol, 1.0);
}
