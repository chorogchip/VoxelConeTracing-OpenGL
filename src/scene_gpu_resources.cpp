#include "scene_gpu_resources.h"

#include <cstddef>
#include <iostream>

#include <glad/glad.h>

#include "graphics_util.h"

namespace {
    constexpr const char* VERTEX_SHADER_PATH = "assets/shaders/scene.vert";
    constexpr const char* FRAGMENT_SHADER_PATH = "assets/shaders/scene.frag";

    uint32_t create_fallback_texture() {
        uint32_t texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        constexpr unsigned char white_pixel[] = { 255, 255, 255, 255 };
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA,
            1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
        return texture;
    }
}


namespace chr {

    int SceneGPUResources::init(const SceneRaw& scene_raw) {
        clear();

        unsigned vertex_shader = graphics_util::compile_shader_from_file(
            GL_VERTEX_SHADER, VERTEX_SHADER_PATH);
        if (vertex_shader == 0) return -1;
        unsigned fragment_shader = graphics_util::compile_shader_from_file(
            GL_FRAGMENT_SHADER, FRAGMENT_SHADER_PATH);
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
        this->uniform_model = glGetUniformLocation(this->shader_program, "model");
        this->uniform_view = glGetUniformLocation(this->shader_program, "view");
        this->uniform_projection = glGetUniformLocation(this->shader_program, "projection");
        this->uniform_texture_diffuse = glGetUniformLocation(this->shader_program, "uTexture");
        this->fallback_texture_diffuse = create_fallback_texture();

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
            const uint32_t texture_diffuse =
                graphics_util::load_texture_2d(material_raw.texture_diffuse);
            this->materials.push_back({
                texture_diffuse != 0 ? texture_diffuse : this->fallback_texture_diffuse,
                0,
                0
            });
        }

        return 0;
    }

    void SceneGPUResources::clear() {
        for (const auto& mesh : this->meshes) {
            glDeleteVertexArrays(1, &mesh.VAO);
            glDeleteBuffers(1, &mesh.VBO);
            glDeleteBuffers(1, &mesh.EBO);
        }
        this->meshes.clear();

        for (const auto& material : this->materials) {
            if (material.texture_diffuse != 0 &&
                material.texture_diffuse != this->fallback_texture_diffuse) {
                glDeleteTextures(1, &material.texture_diffuse);
            }
        }
        this->materials.clear();

        if (this->fallback_texture_diffuse != 0) {
            glDeleteTextures(1, &this->fallback_texture_diffuse);
            this->fallback_texture_diffuse = 0;
        }

        if (this->shader_program != 0) {
            glDeleteProgram(this->shader_program);
            this->shader_program = 0;
        }

        this->uniform_model = -1;
        this->uniform_view = -1;
        this->uniform_projection = -1;
        this->uniform_texture_diffuse = -1;
    }

    void SceneGPUResources::draw(const SceneDrawParams& draw_params)
    {
        glUseProgram(this->shader_program);
        glUniform1i(this->uniform_texture_diffuse, 0);
        glUniformMatrix4fv(
            this->uniform_model,
            1, GL_FALSE, &draw_params.mat_model[0][0]);
        glUniformMatrix4fv(
            this->uniform_view,
            1, GL_FALSE, &draw_params.mat_view[0][0]);
        glUniformMatrix4fv(
            this->uniform_projection,
            1, GL_FALSE, &draw_params.mat_projection[0][0]);

        for (const auto& mesh : this->meshes) {
            uint32_t texture_diffuse = this->fallback_texture_diffuse;
            if (mesh.material_index < this->materials.size()) {
                texture_diffuse = this->materials[mesh.material_index].texture_diffuse;
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_diffuse);
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
        }

        glBindVertexArray(0);
    }

}
