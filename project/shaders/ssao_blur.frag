#version 330 core
in vec2 vUV;

uniform sampler2D uSSAO;
uniform vec2 uTexelSize;

out float FragColor;

void main() {
    float result = 0.0;
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            result += texture(uSSAO, vUV + vec2(float(x), float(y)) * uTexelSize).r;
        }
    }
    FragColor = result / 25.0;
}
