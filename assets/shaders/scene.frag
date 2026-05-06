#version 330 core

layout (location = 0) out vec4 GAlbedo;
layout (location = 1) out vec3 GNormal;

in vec2 TexCoord;
in vec3 Normal;

uniform sampler2D uTexture;
uniform sampler2D uAlphaMaskTexture;

void main() {
    float alpha_mask = texture(uAlphaMaskTexture, TexCoord).r;
    if (alpha_mask < 0.5) {
        discard;
    }

    vec4 albedo = texture(uTexture, TexCoord);
    GAlbedo = albedo;
    GNormal = normalize(Normal);
}
