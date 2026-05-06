#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uTexture;
uniform int uMode;

void main() {
    vec4 sample_value = texture(uTexture, TexCoord);

    if (uMode == 0) {
        FragColor = vec4(sample_value.rgb, 1.0);
        return;
    }

    if (uMode == 1) {
        vec3 normal = normalize(sample_value.rgb);
        FragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    if (uMode == 3) {
        FragColor = vec4(vec3(sample_value.r), 1.0);
        return;
    }

    float depth = sample_value.r;
    float visual_depth = 1.0 - depth;
    FragColor = vec4(vec3(visual_depth), 1.0);
}
