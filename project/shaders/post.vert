#version 330 core

// Tam giác fullscreen (không cần VBO)
const vec2 kPositions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

out vec2 vUV;

void main()
{
    vec2 p = kPositions[gl_VertexID];
    gl_Position = vec4(p, 0.0, 1.0);
    vUV = p * 0.5 + 0.5;
}
