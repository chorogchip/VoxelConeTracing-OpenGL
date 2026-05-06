#include "app_input.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "camera.h"

namespace {
    constexpr float CAMERA_MOVE_SPEED = 0.25f;
    constexpr float MOUSE_SENSITIVITY = 0.08f;

    chr::Camera* g_camera = nullptr;
    bool g_toggle_debug_views_requested = false;
    bool g_toggle_light_markers_requested = false;
    bool g_prev_p_pressed = false;
    bool g_prev_o_pressed = false;
    bool g_is_dragging = false;
    bool g_has_drag_origin = false;
    double g_last_mouse_x = 0.0;
    double g_last_mouse_y = 0.0;

    void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

    void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
        (void)window;

        if (g_camera == nullptr || !g_is_dragging) {
            return;
        }

        if (!g_has_drag_origin) {
            g_last_mouse_x = xpos;
            g_last_mouse_y = ypos;
            g_has_drag_origin = true;
            return;
        }

        const float delta_x = static_cast<float>(xpos - g_last_mouse_x);
        const float delta_y = static_cast<float>(g_last_mouse_y - ypos);
        g_last_mouse_x = xpos;
        g_last_mouse_y = ypos;

        g_camera->move_rotation(delta_x * MOUSE_SENSITIVITY, delta_y * MOUSE_SENSITIVITY);
    }

    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        (void)window;
        (void)mods;

        if (button != GLFW_MOUSE_BUTTON_LEFT) {
            return;
        }

        if (action == GLFW_PRESS) {
            g_is_dragging = true;
            g_has_drag_origin = false;
        } else if (action == GLFW_RELEASE) {
            g_is_dragging = false;
            g_has_drag_origin = false;
        }
    }
}

namespace app_input {

    void initialize(GLFWwindow* window, chr::Camera* camera, int screen_width, int screen_height) {
        (void)screen_width;
        (void)screen_height;

        g_camera = camera;
        g_toggle_debug_views_requested = false;
        g_toggle_light_markers_requested = false;
        g_prev_p_pressed = false;
        g_prev_o_pressed = false;
        g_is_dragging = false;
        g_has_drag_origin = false;

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
    }

    void process_input(GLFWwindow* window) {
        if (g_camera == nullptr) {
            return;
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        const bool is_p_pressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (is_p_pressed && !g_prev_p_pressed) {
            g_toggle_debug_views_requested = true;
        }
        g_prev_p_pressed = is_p_pressed;

        const bool is_o_pressed = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
        if (is_o_pressed && !g_prev_o_pressed) {
            g_toggle_light_markers_requested = true;
        }
        g_prev_o_pressed = is_o_pressed;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            g_camera->position += CAMERA_MOVE_SPEED * g_camera->dir_front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            g_camera->position -= CAMERA_MOVE_SPEED * g_camera->dir_front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            g_camera->position -= glm::normalize(glm::cross(g_camera->dir_front, g_camera->dir_up)) * CAMERA_MOVE_SPEED;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            g_camera->position += glm::normalize(glm::cross(g_camera->dir_front, g_camera->dir_up)) * CAMERA_MOVE_SPEED;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            g_camera->position += CAMERA_MOVE_SPEED * g_camera->dir_up;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            g_camera->position -= CAMERA_MOVE_SPEED * g_camera->dir_up;
    }

    bool consume_toggle_debug_views_requested() {
        const bool requested = g_toggle_debug_views_requested;
        g_toggle_debug_views_requested = false;
        return requested;
    }

    bool consume_toggle_light_markers_requested() {
        const bool requested = g_toggle_light_markers_requested;
        g_toggle_light_markers_requested = false;
        return requested;
    }

}
