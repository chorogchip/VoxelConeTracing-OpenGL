#version 330 core

out float FragColor;

in vec2 TexCoord;

uniform sampler2D gNormal;
uniform sampler2D gDepth;
uniform sampler2D uNoiseTexture;
uniform vec3 uSamples[16];
uniform mat4 uProjection;
uniform mat4 uInverseProjection;
uniform vec2 uNoiseScale;

vec3 reconstruct_view_position(vec2 uv, float depth) {
    vec4 clip_pos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view_pos = uInverseProjection * clip_pos;
    return view_pos.xyz / view_pos.w;
}

void main() {
    float depth = texture(gDepth, TexCoord).r;
    if (depth >= 1.0) {
        FragColor = 1.0;
        return;
    }

    vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
    vec3 view_pos = reconstruct_view_position(TexCoord, depth);
    vec3 random_vec = normalize(texture(uNoiseTexture, TexCoord * uNoiseScale).xyz);

    vec3 tangent = random_vec - normal * dot(random_vec, normal);
    float tangent_length = length(tangent);
    if (tangent_length < 0.0001) {
        tangent = normalize(cross(normal, vec3(0.0, 1.0, 0.0)));
        if (length(tangent) < 0.0001) {
            tangent = normalize(cross(normal, vec3(1.0, 0.0, 0.0)));
        }
    } else {
        tangent /= tangent_length;
    }
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    const float radius = 0.65;
    const float bias = 0.025;

    for (int i = 0; i < 16; ++i) {
        vec3 sample_pos = view_pos + (tbn * uSamples[i]) * radius;
        vec4 clip_sample = uProjection * vec4(sample_pos, 1.0);
        vec3 sample_ndc = clip_sample.xyz / clip_sample.w;
        vec2 sample_uv = sample_ndc.xy * 0.5 + 0.5;

        if (sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0) {
            continue;
        }

        float sample_depth = texture(gDepth, sample_uv).r;
        vec3 sample_view_pos = reconstruct_view_position(sample_uv, sample_depth);
        float range_weight = smoothstep(0.0, 1.0, radius / (abs(view_pos.z - sample_view_pos.z) + 0.0001));
        occlusion += (sample_view_pos.z >= sample_pos.z + bias ? 1.0 : 0.0) * range_weight;
    }

    FragColor = 1.0 - (occlusion / 16.0);
}
