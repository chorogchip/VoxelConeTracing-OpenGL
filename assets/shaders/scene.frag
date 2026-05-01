#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;

uniform sampler2D uTexture;

void main() {
    vec3 debugNormal = normalize(Normal) * 0.5 + 0.5;
    FragColor = texture(uTexture, TexCoord);
}
