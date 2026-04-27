#version 330 core
in float vAlpha;
in float vType;
out vec4 FragColor;

void main() {
    // gl_PointCoord is the coordinate within the point sprite (0.0 to 1.0)
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    if (dist > 0.5) {
        discard;
    }
    
    float alpha = 0.0;
    vec3 color = vec3(1.0);
    
    if (vType < 0.5) {
        // Rain (Type 0)
        // Rain is more streak-like. We will use a soft circle for now but lower max alpha.
        alpha = smoothstep(0.5, 0.2, dist) * vAlpha * 0.4;
        color = vec3(0.8, 0.9, 1.0);
    } else {
        // Smoke (Type 1)
        // Smoke is puffier, more solid in center but still very transparent overall.
        alpha = smoothstep(0.5, 0.0, dist) * vAlpha * 0.8;
        // Smoke color (slightly yellowish-gray due to city lights)
        color = vec3(0.85, 0.85, 0.82);
    }
    
    FragColor = vec4(color, alpha);
}
