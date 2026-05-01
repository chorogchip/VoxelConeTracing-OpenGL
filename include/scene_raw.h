#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

namespace chr {

    struct SceneRaw {
        struct Mesh {
            struct Vertex {
                glm::vec3 position;
                glm::vec2 tex_coord;
                glm::vec3 normal;
                glm::vec3 tangent;
                glm::vec3 bitangent;
            };
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            uint32_t material_index;
        };
        struct Material {
            std::string texture_diffuse;
            std::string texture_normal;
            std::string texture_alpha_mask;
        };
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
    };

}