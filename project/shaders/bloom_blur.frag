#version 330 core
in vec2 vUV;
uniform sampler2D uInput;
uniform vec2 uDirection;   // e.g. (1/w, 0) or (0, 1/h)
out vec4 FragColor;
void main()
{
    // 9-tap Gaussian
    float w[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 result = texture(uInput, vUV).rgb * w[0];
    for (int i = 1; i < 5; i++)
    {
        vec2 off = uDirection * float(i);
        result += texture(uInput, vUV + off).rgb * w[i];
        result += texture(uInput, vUV - off).rgb * w[i];
    }
    FragColor = vec4(result, 1.0);
}
