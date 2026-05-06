#pragma once

#include <vector>
#include <cstdint>

#include <glm/glm.hpp>

#include "scene_raw.h"

namespace chr {

    struct SceneDrawParams {
        glm::mat4 mat_model;
        glm::mat4 mat_view;
        glm::mat4 mat_projection;
    };

    struct SceneGPUResources {
        struct Mesh {
            uint32_t VAO;
            uint32_t VBO;
            uint32_t EBO;
            uint32_t index_count;
            uint32_t material_index;
        };
        struct Material {
            uint32_t texture_diffuse;
            uint32_t texture_normal;
            uint32_t texture_alpha_mask;
        };
        std::vector<Mesh> meshes;
        std::vector<Material> materials;

        uint32_t shader_program = 0;
        uint32_t shadow_shader_program = 0;
        uint32_t fallback_texture_diffuse = 0;
        int uniform_model = -1;
        int uniform_view = -1;
        int uniform_projection = -1;
        int uniform_texture_diffuse = -1;
        int uniform_texture_alpha_mask = -1;
        int uniform_shadow_model = -1;
        int uniform_shadow_light_view_projection = -1;
        int uniform_shadow_texture_alpha_mask = -1;
    };

}
