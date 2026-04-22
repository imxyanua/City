#version 330 core
in float vAlpha;
out vec4 FragColor;

uniform float uRainIntensity;

void main()
{
    // Light blueish-white rain color
    vec3 color = vec3(0.8, 0.85, 0.95);
    FragColor = vec4(color, vAlpha * uRainIntensity * 0.5);
}
