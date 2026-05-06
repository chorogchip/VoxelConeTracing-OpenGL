#pragma once

#include "scene_gpu_resources.h"

namespace chr {

    int init_scene_gpu_resources(SceneGPUResources* resources, const SceneRaw& scene_raw);
    void clear_scene_gpu_resources(SceneGPUResources* resources);
    void render_scene_gpu_resources(
        const SceneGPUResources& resources,
        const SceneDrawParams& draw_params);
    void render_scene_gpu_resources_shadow(
        const SceneGPUResources& resources,
        const glm::mat4& mat_model,
        const glm::mat4& mat_light_view_projection);

}
