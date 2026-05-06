#version 330 core

layout (location = 0) out vec4 GAlbedo;
layout (location = 1) out vec3 GNormal;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;

uniform sampler2D uTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uAlphaMaskTexture;

void main() {
    float alpha_mask = texture(uAlphaMaskTexture, TexCoord).r;
    if (alpha_mask < 0.5) {
        discard;
    }

    vec4 albedo = texture(uTexture, TexCoord);
    vec3 base_normal = normalize(Normal);
    vec3 tangent = Tangent;
    tangent = tangent - dot(tangent, base_normal) * base_normal;
    float tangent_length = length(tangent);

    vec3 final_normal = base_normal;
    if (tangent_length > 0.0001) {
        tangent /= tangent_length;
        vec3 bitangent = normalize(cross(base_normal, tangent));
        vec3 sampled_normal = texture(uNormalTexture, TexCoord).rgb * 2.0 - 1.0;
        mat3 tbn = mat3(tangent, bitangent, base_normal);
        final_normal = normalize(tbn * sampled_normal);
    }

    GAlbedo = albedo;
    GNormal = final_normal;
}
