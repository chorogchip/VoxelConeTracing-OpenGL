#include "scene_gpu_resources.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <stb_image.h>

#include "graphics_util.h"

namespace {

    static constexpr char* VERTEX_SHADER_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    Normal = mat3(transpose(inverse(model))) * aNormal;
}
)";

    static constexpr char* FRAGMENT_SHADER_SOURCE = R"(
#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;

uniform sampler2D uTexture;

void main() {
    vec3 debugNormal = normalize(Normal) * 0.5 + 0.5;
    FragColor = texture(uTexture, TexCoord);
}
)";
}


namespace chr {

    int SceneGPUResources::init(const SceneRaw& scene_raw) {

        unsigned vertex_shader = graphics_util::compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE);
        if (vertex_shader == 0) return -1;
        unsigned fragment_shader = graphics_util::compile_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE);
        if (fragment_shader == 0) {
            glDeleteShader(vertex_shader);
            return -1;
        }

        unsigned shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);

        int succeed;
        glGetProgramiv(shader_program, GL_LINK_STATUS, &succeed);
        if (!succeed) {
            char log_buf[512];
            glGetProgramInfoLog(shader_program, 512, nullptr, log_buf);
            std::cout << "Err: Shader program link failed: " << log_buf << std::endl;
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            return -1;
        }
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        this->shader_program = shader_program;

        for (const auto& mesh_raw : scene_raw.meshes) {

            unsigned VAO, VBO, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            this->meshes.push_back({ VAO, VBO, EBO,
                static_cast<uint32_t>(mesh_raw.indices.size()),
                mesh_raw.material_index });

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, mesh_raw.vertices.size() * sizeof(mesh_raw.vertices[0]),
                mesh_raw.vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_raw.indices.size() * sizeof(mesh_raw.indices[0]),
                mesh_raw.indices.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
                (void*)offsetof(chr::SceneRaw::Mesh::Vertex, position));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
                (void*)offsetof(chr::SceneRaw::Mesh::Vertex, tex_coord));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
                (void*)offsetof(chr::SceneRaw::Mesh::Vertex, normal));
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
                (void*)offsetof(chr::SceneRaw::Mesh::Vertex, tangent));
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
                (void*)offsetof(chr::SceneRaw::Mesh::Vertex, bitangent));
            glBindVertexArray(0);
        }

        for (const auto& material_raw : scene_raw.materials) {

            unsigned texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            {
                int tex_width, tex_height, tex_channels;
                stbi_set_flip_vertically_on_load(true);
                unsigned char* data = stbi_load("assets/texture.png",
                    &tex_width, &tex_height, &tex_channels, 0);
                if (data != nullptr) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    stbi_image_free(data);
                }
                else {
                    std::cout << "Failed to load texture" << std::endl;
                    return -1;
                }
            }
        }

        return 0;
    }

    void SceneGPUResources::clear() {

    }

    void SceneGPUResources::bind() {

    }

}