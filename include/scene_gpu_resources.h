#pragma once

#include <vector>
#include <cstdint>

#include "scene_raw.h"

namespace chr {

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

        uint32_t shader_program;

        int init(const SceneRaw& scene_raw);
        void clear();
        void bind();
    };

}