#version 330 core
in float vAlpha;
out vec4 FragColor;

uniform float uRainIntensity;

void main()
{
    vec3 color = vec3(0.78, 0.86, 0.95);
    float alpha = vAlpha * uRainIntensity;
    FragColor = vec4(color, alpha);
}
