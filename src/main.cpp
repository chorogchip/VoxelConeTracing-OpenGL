#include <iostream>
#include <immintrin.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "app_input.h"
#include "camera.h"
#include "g_buffer_resources.h"
#include "imgui_layer.h"
#include "model_loader.h"
#include "scene_gpu_resources.h"
#include "scene_gpu_resources_runtime.h"

constexpr unsigned SCREEN_WIDTH = 800;
constexpr unsigned SCREEN_HEIGHT = 600;
constexpr const char* SPONZA_OBJ_RELATIVE_PATH = "assets/Sponza-master/sponza.obj";

chr::Camera camera{};
bool show_debug_views = false;
bool show_light_markers = true;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Voxel Cone Tracing", NULL, NULL);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    app_input::initialize(window, &camera, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    if (imgui_layer::init(window) != 0) {
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    int framebuffer_width = SCREEN_WIDTH;
    int framebuffer_height = SCREEN_HEIGHT;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    chr::GBufferResources g_buffer_resources;
    if (g_buffer_resources.init(framebuffer_width, framebuffer_height) != 0) {
        g_buffer_resources.clear();
        imgui_layer::shutdown();
        glfwTerminate();
        return -1;
    }

    chr::SceneRaw scene_raw = chr::load_obj(SPONZA_OBJ_RELATIVE_PATH);
    if (scene_raw.meshes.empty()) {
        g_buffer_resources.clear();
        imgui_layer::shutdown();
        std::cout << "Failed to load scene data from: " << SPONZA_OBJ_RELATIVE_PATH << std::endl;
        glfwTerminate();
        return -1;
    }

    chr::SceneGPUResources scene_gpu_resources;
    if (chr::init_scene_gpu_resources(&scene_gpu_resources, scene_raw) != 0) {
        chr::clear_scene_gpu_resources(&scene_gpu_resources);
        g_buffer_resources.clear();
        imgui_layer::shutdown();
        glfwTerminate();
        return -1;
    }

    camera.set_position(glm::vec3(0.0f, 20.0f, 50.0f));
    camera.set_lookat(glm::vec3(0.0f, 5.0f, 0.0f));

    float last_time = static_cast<float>(glfwGetTime());
    uint64_t frames = 0;
    while (!glfwWindowShouldClose(window)) {
        
        float cur_time = static_cast<float>(glfwGetTime());
        float delta_time = cur_time - last_time;
        if (delta_time < 1.0f / 60.0f) {
            _mm_pause();
            continue;
        } else if (delta_time > 2.0f / 60.0f) {
            last_time = cur_time;
        } else {
            last_time = cur_time;
        }

        app_input::process_input(window);
        if (app_input::consume_toggle_debug_views_requested()) {
            show_debug_views = !show_debug_views;
        }
        if (app_input::consume_toggle_light_markers_requested()) {
            show_light_markers = !show_light_markers;
        }

        int current_framebuffer_width = 0;
        int current_framebuffer_height = 0;
        glfwGetFramebufferSize(window, &current_framebuffer_width, &current_framebuffer_height);
        if (current_framebuffer_width <= 0 || current_framebuffer_height <= 0) {
            glfwPollEvents();
            continue;
        }

        if (current_framebuffer_width != framebuffer_width || current_framebuffer_height != framebuffer_height) {
            if (g_buffer_resources.resize(current_framebuffer_width, current_framebuffer_height) != 0) {
                break;
            }
            framebuffer_width = current_framebuffer_width;
            framebuffer_height = current_framebuffer_height;
        }

        imgui_layer::begin_frame();

        chr::SceneDrawParams draw_params{};
        draw_params.mat_projection = camera.get_projection_matrix(
            static_cast<float>(framebuffer_width) / framebuffer_height);
        draw_params.mat_view = camera.get_view_matrix();
        draw_params.mat_model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));

        g_buffer_resources.bind_for_shadow_pass();
        glClear(GL_DEPTH_BUFFER_BIT);
        chr::render_scene_gpu_resources_shadow(
            scene_gpu_resources,
            draw_params.mat_model,
            g_buffer_resources.get_directional_light_view_projection());

        g_buffer_resources.bind_for_geometry_pass();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        chr::render_scene_gpu_resources(scene_gpu_resources, draw_params);
        g_buffer_resources.draw_lighting_pass(draw_params.mat_projection, draw_params.mat_view);
        if (show_light_markers) {
            g_buffer_resources.draw_light_markers(draw_params.mat_projection, draw_params.mat_view);
        }
        if (show_debug_views) {
            g_buffer_resources.draw_debug_views();
        }
        imgui_layer::draw_overlay();
        imgui_layer::end_frame();

        glfwSwapBuffers(window);
        glfwPollEvents();

        ++frames;
    }

    chr::clear_scene_gpu_resources(&scene_gpu_resources);
    g_buffer_resources.clear();
    imgui_layer::shutdown();
    glfwTerminate();
    return 0;
}
