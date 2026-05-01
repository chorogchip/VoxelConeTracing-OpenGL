#include "model_loader.h"

#include <filesystem>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>


namespace chr {

    namespace {

        std::string get_texture_path(
            const aiScene* scene,
            aiMaterial* material,
            aiTextureType type,
            const std::filesystem::path& model_dir)
        {
            if (material == nullptr || material->GetTextureCount(type) == 0) {
                return {};
            }

            aiString path;
            if (material->GetTexture(type, 0, &path) != aiReturn_SUCCESS) {
                return {};
            }

            std::filesystem::path texture_path = path.C_Str();

            if (texture_path.is_absolute()) {
                return texture_path.string();
            }

            return (model_dir / texture_path).lexically_normal().string();
        }

    }

    SceneRaw load_obj(const char* filename) {
        SceneRaw result;
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            filename,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
            return result;
        }

        const std::filesystem::path model_path(filename);
        const std::filesystem::path model_dir = model_path.parent_path();

        result.materials.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            aiMaterial* material = scene->mMaterials[i];

            SceneRaw::Material out_material{};

            out_material.texture_diffuse =
                get_texture_path(scene, material, aiTextureType_DIFFUSE, model_dir);

            out_material.texture_normal =
                get_texture_path(scene, material, aiTextureType_NORMALS, model_dir);

            if (out_material.texture_normal.empty()) {
                out_material.texture_normal =
                    get_texture_path(scene, material, aiTextureType_HEIGHT, model_dir);
            }

            // alpha mask
            out_material.texture_alpha_mask =
                get_texture_path(scene, material, aiTextureType_OPACITY, model_dir);

            result.materials.push_back(std::move(out_material));
        }

        result.meshes.reserve(scene->mNumMeshes);
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[i];

            SceneRaw::Mesh out_mesh{};
            out_mesh.material_index = mesh->mMaterialIndex;

            out_mesh.vertices.reserve(mesh->mNumVertices);
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                SceneRaw::Mesh::Vertex vertex{};

                vertex.position = {
                    mesh->mVertices[j].x,
                    mesh->mVertices[j].y,
                    mesh->mVertices[j].z
                };

                if (mesh->HasTextureCoords(0)) {
                    vertex.tex_coord = {
                        mesh->mTextureCoords[0][j].x,
                        mesh->mTextureCoords[0][j].y
                    };
                }
                else {
                    vertex.tex_coord = { 0.0f, 0.0f };
                }

                if (mesh->HasNormals()) {
                    vertex.normal = {
                        mesh->mNormals[j].x,
                        mesh->mNormals[j].y,
                        mesh->mNormals[j].z
                    };
                }
                else {
                    vertex.normal = { 0.0f, 0.0f, 0.0f };
                }

                if (mesh->HasTangentsAndBitangents()) {
                    vertex.tangent = {
                        mesh->mTangents[j].x,
                        mesh->mTangents[j].y,
                        mesh->mTangents[j].z
                    };
                    vertex.bitangent = {
                        mesh->mBitangents[j].x,
                        mesh->mBitangents[j].y,
                        mesh->mBitangents[j].z
                    };
                }
                else {
                    vertex.tangent = { 0.0f, 0.0f, 0.0f };
                    vertex.bitangent = { 0.0f, 0.0f, 0.0f };
                }

                out_mesh.vertices.push_back(vertex);
            }

            out_mesh.indices.reserve(mesh->mNumFaces * 3);
            for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
                const aiFace& face = mesh->mFaces[j];

                for (unsigned int k = 0; k < face.mNumIndices; ++k) {
                    out_mesh.indices.push_back(face.mIndices[k]);
                }
            }

            result.meshes.push_back(std::move(out_mesh));
        }

        std::cout << "Successfully loaded: " << filename << std::endl;
        std::cout << "Total Meshes: " << result.meshes.size() << std::endl;
        std::cout << "Total Materials: " << result.materials.size() << std::endl;

        size_t total_vertices = 0;
        size_t total_indices = 0;
        for (const auto& mesh : result.meshes) {
            total_vertices += mesh.vertices.size();
            total_indices += mesh.indices.size();
        }

        std::cout << "Total Vertices: " << total_vertices << std::endl;
        std::cout << "Total Indices: " << total_indices << std::endl;

        return result;
    }

}