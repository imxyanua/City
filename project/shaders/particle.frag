#version 330 core
in float vAlpha;
out vec4 FragColor;

void main() {
    // gl_PointCoord is the coordinate within the point sprite (0.0 to 1.0)
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    if (dist > 0.5) {
        discard;
    }
    
    // Soft edge
    float alpha = smoothstep(0.5, 0.2, dist) * vAlpha * 0.4; // 0.4 max alpha to look like soft water mist
    
    // Water color (slightly blue/white)
    vec3 color = vec3(0.8, 0.9, 1.0);
    
    FragColor = vec4(color, alpha);
}
