#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aSize;
layout (location = 2) in float aAlpha;

uniform mat4 uProjection;
uniform mat4 uView;

out float vAlpha;

void main() {
    vAlpha = aAlpha;
    vec4 viewPos = uView * vec4(aPos, 1.0);
    gl_Position = uProjection * viewPos;
    // Calculate point size depending on distance (viewPos.z is negative)
    gl_PointSize = (aSize * 300.0) / max(abs(viewPos.z), 1.0);
}
