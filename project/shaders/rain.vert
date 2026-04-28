#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aInstancePos;
layout (location = 2) in vec2 aParams; // x: length, y: width

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec3 uCameraPos;
uniform vec2 uWindDir;

out float vAlpha;

void main()
{
    float length = aParams.x;
    float width = aParams.y;

    vec3 dir = normalize(vec3(uWindDir.x * 0.35, -1.0, uWindDir.y * 0.35));
    vec3 toCamera = normalize(uCameraPos - aInstancePos);
    vec3 right = normalize(cross(dir, toCamera));

    vec3 finalPos = aInstancePos
        + right * (aPos.x * width)
        + dir * (aPos.y * length);

    float dist = length(aInstancePos - uCameraPos);
    vAlpha = 1.0 - smoothstep(30.0, 200.0, dist);

    gl_Position = uProjection * uView * vec4(finalPos, 1.0);
}
