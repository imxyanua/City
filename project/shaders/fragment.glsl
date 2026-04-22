#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

// Sun: direction of incoming light rays in world space (same convention as before: L = normalize(-uLightDir)).
uniform vec3 uLightDir;
uniform vec3 uSkyColor;
uniform vec3 uGroundColor;
uniform vec3 uSunColor;
uniform vec3 uViewPos;
uniform float uExposure;
uniform vec3 uFogColor;
uniform float uFogDensity;

uniform vec4 uBaseColorFactor;
uniform sampler2D uDiffuseTex;
uniform bool uUseTexture;

out vec4 FragColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Albedo in linear space (sampler uses sRGB texture → hardware decodes to linear).
    vec3 albedo = uBaseColorFactor.rgb;
    if (uUseTexture)
    {
        vec3 tex = texture(uDiffuseTex, vTexCoord).rgb;
        albedo *= tex;
    }

    // Hemisphere ambient: sky on up-facing normals, warm ground bounce below (điển hình phố).
    float hemi = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambLight = mix(uGroundColor, uSkyColor, hemi);
    vec3 ambientTerm = albedo * ambLight;

    // Mềm vùng tối (wrap diffuse) để không bị “cháy đen” giữa các khối.
    const float wrap = 0.12;
    float NdotL = max(dot(N, L), 0.0);
    float diff = clamp((NdotL + wrap) / (1.0 + wrap), 0.0, 1.0);
    vec3 diffuseTerm = albedo * uSunColor * diff;

    // Blinn–Phong, highlight nhẹ kiểu bê tông / sơn đô thị.
    float specMask = pow(max(dot(N, H), 0.0), 48.0);
    vec3 specColor = mix(vec3(1.0), uSunColor * 0.35, 0.5);
    vec3 specularTerm = specColor * specMask * 0.22;

    vec3 color = ambientTerm + diffuseTerm + specularTerm;

    // Sương mù theo khoảng cách (exponential), mật độ 0 = tắt.
    if (uFogDensity > 1e-6)
    {
        float dist = length(uViewPos - vWorldPos);
        float fogAmt = 1.0 - exp(-uFogDensity * dist * 0.045);
        fogAmt = clamp(fogAmt, 0.0, 1.0);
        color = mix(color, uFogColor, fogAmt);
    }

    // Exposure (linear); framebuffer sRGB sẽ chuyển sang gamma hiển thị.
    color *= uExposure;
    // Viền sáng nhẹ (Reinhard đơn giản) để màu không bị “cứng”.
    color = color / (color + vec3(0.92));

    FragColor = vec4(clamp(color, 0.0, 1.0), uBaseColorFactor.a);
}
