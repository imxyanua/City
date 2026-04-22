#version 330 core
layout (location = 0) in vec3 aPos;     // base geometry of rain streak, e.g. (0,0,0) and (0,-1,0) // actually length is scaled in Y
layout (location = 1) in vec3 aOffset;  // random starting pos in the box

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec3 uCameraPos;
uniform float uTime;
uniform vec2 uWindDir;
uniform float uFallSpeed;
uniform float uRainLength;

out float vAlpha;

void main()
{
    vec3 pos = aOffset;
    
    // Animate falling
    pos.y -= uTime * uFallSpeed;
    // Animate wind
    pos.x += uWindDir.x * uTime * (uFallSpeed * 0.3);
    pos.z += uWindDir.y * uTime * (uFallSpeed * 0.3);
    
    // Wrap around camera inside a box: X/Z in [-75, 75], Y in [-20, 100]
    float boxSizeXZ = 150.0;
    float boxSizeY  = 120.0;
    
    pos.x = mod(pos.x - uCameraPos.x + boxSizeXZ * 0.5, boxSizeXZ) - boxSizeXZ * 0.5 + uCameraPos.x;
    pos.z = mod(pos.z - uCameraPos.z + boxSizeXZ * 0.5, boxSizeXZ) - boxSizeXZ * 0.5 + uCameraPos.z;
    pos.y = mod(pos.y - uCameraPos.y + 40.0, boxSizeY) - 20.0 + uCameraPos.y;
    
    // Calculate streak velocity vector
    vec3 velocity = vec3(uWindDir.x * 0.3, -1.0, uWindDir.y * 0.3);
    velocity = normalize(velocity);
    
    vec3 finalPos = pos + aPos * uRainLength * velocity;
    
    // Distance fading
    float dist = length(finalPos - uCameraPos);
    vAlpha = 1.0 - smoothstep(10.0, 100.0, dist);
    
    gl_Position = uProjection * uView * vec4(finalPos, 1.0);
}
