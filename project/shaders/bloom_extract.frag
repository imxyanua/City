#version 330 core
in vec2 vUV;
uniform sampler2D uScene;
uniform float uThreshold;
out vec4 FragColor;
void main()
{
    vec3  c    = texture(uScene, vUV).rgb;
    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3  bright = c * smoothstep(uThreshold - 0.1, uThreshold + 0.4, luma);
    FragColor = vec4(bright, 1.0);
}
