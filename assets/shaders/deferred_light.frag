#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D gAlbedo;
uniform sampler2D gNormal;
uniform sampler2D gDepth;
uniform sampler2D uShadowMap;
uniform sampler2D uSsaoMap;
uniform mat4 uInverseProjection;
uniform mat4 uInverseView;
uniform mat4 uLightViewProjection;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform int uPointLightCount;
uniform vec3 uPointLightPositions[5];
uniform vec3 uPointLightColors[5];
uniform float uPointLightIntensities[5];
uniform float uPointLightRanges[5];

vec3 reconstruct_view_position(vec2 uv, float depth) {
    vec4 clip_pos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view_pos = uInverseProjection * clip_pos;
    return view_pos.xyz / view_pos.w;
}

float calculate_directional_shadow(vec3 view_pos, vec3 normal) {
    vec3 world_pos = (uInverseView * vec4(view_pos, 1.0)).xyz;
    vec4 light_clip = uLightViewProjection * vec4(world_pos, 1.0);
    vec3 light_ndc = light_clip.xyz / light_clip.w;
    vec3 shadow_uvz = light_ndc * 0.5 + 0.5;

    if (shadow_uvz.z < 0.0 || shadow_uvz.z > 1.0) {
        return 0.0;
    }
    if (shadow_uvz.x < 0.0 || shadow_uvz.x > 1.0 || shadow_uvz.y < 0.0 || shadow_uvz.y > 1.0) {
        return 0.0;
    }

    vec3 view_light_dir = normalize(-uLightDirection);
    float bias = max(0.0015 * (1.0 - dot(normal, view_light_dir)), 0.0005);
    vec2 texel_size = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float closest_depth = texture(uShadowMap, shadow_uvz.xy + vec2(x, y) * texel_size).r;
            shadow += shadow_uvz.z - bias > closest_depth ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

void main() {
    vec4 albedo = texture(gAlbedo, TexCoord);
    vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
    float depth = texture(gDepth, TexCoord).r;
    float ambient_occlusion = texture(uSsaoMap, TexCoord).r;

    vec3 view_pos = reconstruct_view_position(TexCoord, depth);
    vec3 light_dir = normalize(-uLightDirection);
    float lambert = max(dot(normal, light_dir), 0.0);
    float directional_shadow = calculate_directional_shadow(view_pos, normal);

    vec3 ambient = ambient_occlusion * uAmbientStrength * uLightColor;
    vec3 diffuse = (1.0 - directional_shadow) * lambert * uDiffuseStrength * uLightColor;
    vec3 point_light_sum = vec3(0.0);

    for (int i = 0; i < uPointLightCount; ++i) {
        vec3 to_light = uPointLightPositions[i] - view_pos;
        float distance_to_light = length(to_light);
        if (distance_to_light >= uPointLightRanges[i]) {
            continue;
        }

        vec3 point_light_dir = to_light / max(distance_to_light, 0.0001);
        float point_lambert = max(dot(normal, point_light_dir), 0.0);
        float attenuation = 1.0 - (distance_to_light / uPointLightRanges[i]);
        attenuation *= attenuation;
        point_light_sum += point_lambert * attenuation * uPointLightIntensities[i] * uPointLightColors[i];
    }

    vec3 lit_color = (ambient + diffuse + point_light_sum) * albedo.rgb;

    FragColor = vec4(lit_color, albedo.a);
}
