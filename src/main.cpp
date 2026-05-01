#include <iostream>
#include <immintrin.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "model_loader.h"
#include "scene_gpu_resources.h"

void process_input(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

constexpr unsigned SCREEN_WIDTH = 800;
constexpr unsigned SCREEN_HEIGHT = 600;
constexpr const char* SPONZA_OBJ_RELATIVE_PATH = "assets/Sponza-master/sponza.obj";
constexpr float CAMERA_MOVE_SPEED = 0.25f;
constexpr float MOUSE_SENSITIVITY = 0.08f;
constexpr float SCROLL_SENSITIVITY = 2.0f;

chr::Camera camera{};
bool is_mouse_captured = true;
bool first_mouse_input = true;
double last_mouse_x = static_cast<double>(SCREEN_WIDTH) * 0.5;
double last_mouse_y = static_cast<double>(SCREEN_HEIGHT) * 0.5;

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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    chr::SceneRaw scene_raw = chr::load_obj(SPONZA_OBJ_RELATIVE_PATH);
    if (scene_raw.meshes.empty()) {
        std::cout << "Failed to load scene data from: " << SPONZA_OBJ_RELATIVE_PATH << std::endl;
        glfwTerminate();
        return -1;
    }

    chr::SceneGPUResources scene_gpu_resources;
    if (scene_gpu_resources.init(scene_raw) != 0) {
        scene_gpu_resources.clear();
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

        process_input(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        chr::SceneDrawParams draw_params{};
        draw_params.mat_projection = camera.get_projection_matrix(
            static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT);
        draw_params.mat_view = camera.get_view_matrix();
        draw_params.mat_model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
        scene_gpu_resources.draw(draw_params);

        glfwSwapBuffers(window);
        glfwPollEvents();

        ++frames;
    }

    scene_gpu_resources.clear();
    glfwTerminate();
    return 0;
}

void process_input(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        if (is_mouse_captured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            is_mouse_captured = false;
            first_mouse_input = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.position += CAMERA_MOVE_SPEED * camera.dir_front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.position -= CAMERA_MOVE_SPEED * camera.dir_front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.position -= glm::normalize(glm::cross(camera.dir_front, camera.dir_up)) * CAMERA_MOVE_SPEED;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.position += glm::normalize(glm::cross(camera.dir_front, camera.dir_up)) * CAMERA_MOVE_SPEED;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.position += CAMERA_MOVE_SPEED * camera.dir_up;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.position -= CAMERA_MOVE_SPEED * camera.dir_up;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!is_mouse_captured) {
        return;
    }

    if (first_mouse_input) {
        last_mouse_x = xpos;
        last_mouse_y = ypos;
        first_mouse_input = false;
        return;
    }

    const float delta_x = static_cast<float>(xpos - last_mouse_x);
    const float delta_y = static_cast<float>(last_mouse_y - ypos);
    last_mouse_x = xpos;
    last_mouse_y = ypos;

    camera.move_rotation(delta_x * MOUSE_SENSITIVITY, delta_y * MOUSE_SENSITIVITY);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.move_zoom(static_cast<float>(-yoffset) * SCROLL_SENSITIVITY);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !is_mouse_captured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        is_mouse_captured = true;
        first_mouse_input = true;
    }
}
