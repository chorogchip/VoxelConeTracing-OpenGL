#version 330 core

in vec2 TexCoord;

uniform sampler2D uAlphaMaskTexture;

void main() {
    float alpha_mask = texture(uAlphaMaskTexture, TexCoord).r;
    if (alpha_mask < 0.5) {
        discard;
    }
}
