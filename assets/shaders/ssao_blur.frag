#version 330 core

out float FragColor;

in vec2 TexCoord;

uniform sampler2D uTexture;

void main() {
    vec2 texel_size = 1.0 / vec2(textureSize(uTexture, 0));
    float value = 0.0;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            value += texture(uTexture, TexCoord + vec2(x, y) * texel_size).r;
        }
    }

    FragColor = value / 9.0;
}
