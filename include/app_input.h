#pragma once

struct GLFWwindow;

namespace chr {
    class Camera;
}

namespace app_input {
    void initialize(GLFWwindow* window, chr::Camera* camera, int screen_width, int screen_height);
    void process_input(GLFWwindow* window);
    bool consume_toggle_debug_views_requested();
    bool consume_toggle_light_markers_requested();
}
