#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace chr {
    class Camera {
    public:
        glm::vec3 position;

        glm::vec3 dir_front;
        glm::vec3 dir_up;
        glm::vec3 dir_right;

        float yaw_degree;
        float pitch_degree;

        float zoom;

        Camera() :
            position(0.0f, 0.0f, 0.0f),
            dir_front(0.0f, 0.0f, -1.0f),
            dir_up(0.0f, 1.0f, 0.0f),
            dir_right(1.0f, 0.0f, 0.0f),
            yaw_degree(-90.0f),
            pitch_degree(0.0f),
            zoom(45.0f)
        {}

        void set_position(glm::vec3 pos) {
            position = pos;
        }

        void set_lookat(glm::vec3 target) {
            constexpr float EPS = 0.00001f;

            glm::vec3 gap = target - position;
            if (glm::length(gap) < EPS) return;
            dir_front = glm::normalize(gap);

            glm::vec3 temp_world_up = glm::vec3(0.0f, 1.0f, 0.0f);
            if (glm::abs(glm::dot(dir_front, temp_world_up)) > 1.0f - EPS) {
                temp_world_up = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            dir_right = glm::normalize(glm::cross(dir_front, temp_world_up));
            dir_up = glm::normalize(glm::cross(dir_right, dir_front));

            yaw_degree = glm::degrees(atan2(dir_front.z, dir_front.x));
            pitch_degree = glm::degrees(asin(glm::clamp(dir_front.y, -1.0f + EPS, 1.0f - EPS)));
        }

        void move_to_forward(float distance) {
            position += dir_front * distance;
        }

        void move_rotation(float delta_yaw_degree, float delta_pitch_degree) {
            constexpr float EPS = 0.00001f;

            yaw_degree = glm::mod(yaw_degree + delta_yaw_degree + 180.0f, 360.0f);
            if (yaw_degree < 0.0f) yaw_degree += 360.0f;
            yaw_degree -= 180.0f;

            pitch_degree = glm::clamp(pitch_degree + delta_pitch_degree,
                -90.0f + EPS, 90.0f - EPS);

            float yaw_rad = glm::radians(yaw_degree);
            float pitch_rad = glm::radians(pitch_degree);

            dir_front.x = cos(yaw_rad) * cos(pitch_rad);
            dir_front.y = sin(pitch_rad);
            dir_front.z = sin(yaw_rad) * cos(pitch_rad);
            dir_front = glm::normalize(dir_front);

            glm::vec3 temp_world_up = glm::vec3(0.0f, 1.0f, 0.0f);
            if (glm::abs(glm::dot(dir_front, temp_world_up)) > 1.0f - EPS) {
                temp_world_up = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            dir_right = glm::normalize(glm::cross(dir_front, temp_world_up));
            dir_up = glm::normalize(glm::cross(dir_right, dir_front));
        }

        void set_zoom(float zoom_value) {
            zoom = glm::clamp(zoom_value, 1.0f, 45.0f);
        }

        void move_zoom(float zoom_delta) {
            zoom = glm::clamp(zoom + zoom_delta, 1.0f, 45.0f);
        }

        glm::mat4 get_view_matrix() {
            return glm::lookAt(position, position + dir_front, dir_up);
        }

        glm::mat4 get_projection_matrix(float aspect_ratio) {
            return glm::perspective(zoom, aspect_ratio, 0.1f, 100.0f);
        }

    };
}