#version 330 core
in vec2 vUV;

uniform sampler2D uDepth;
uniform sampler2D uNormal;
uniform sampler2D uNoiseTex;

uniform vec3  uKernel[32];
uniform mat4  uProjection;
uniform mat4  uInvProjection;
uniform vec2  uResolution;
uniform float uRadius;
uniform float uBias;

out float FragColor;

vec3 viewPosFromDepth(vec2 uv, float depth) {
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 clip = vec4(ndc, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = uInvProjection * clip;
    return viewPos.xyz / viewPos.w;
}

void main() {
    float depth = texture(uDepth, vUV).r;
    if (depth >= 0.9999) { FragColor = 1.0; return; } // sky

    vec3 fragPos = viewPosFromDepth(vUV, depth);
    vec3 normal = normalize(texture(uNormal, vUV).rgb * 2.0 - 1.0);

    // Tile noise texture over screen (4x4 noise texture)
    vec2 noiseScale = uResolution / 4.0;
    vec3 randomVec = texture(uNoiseTex, vUV * noiseScale).xyz * 2.0 - 1.0;

    // Gram-Schmidt to build TBN
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < 32; i++) {
        vec3 samplePos = fragPos + TBN * uKernel[i] * uRadius;

        // Project sample to screen
        vec4 offset = uProjection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(uDepth, offset.xy).r;
        vec3 sampleViewPos = viewPosFromDepth(offset.xy, sampleDepth);

        // Range check + occlusion
        float rangeCheck = smoothstep(0.0, 1.0, uRadius / abs(fragPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + uBias ? 1.0 : 0.0) * rangeCheck;
    }

    FragColor = 1.0 - (occlusion / 32.0);
}
