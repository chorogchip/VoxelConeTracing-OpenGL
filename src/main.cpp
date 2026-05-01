
#include <iostream>
#include <immintrin.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "camera.h"
#include "graphics_util.h"
#include "vertices.h"
#include "model_loader.h"

void process_input(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

constexpr unsigned SCREEN_WIDTH = 800;
constexpr unsigned SCREEN_HEIGHT = 600;

chr::Camera camera{};

constexpr char* VERTEX_SHADER_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    Normal = mat3(transpose(inverse(model))) * aNormal;
}
)";

constexpr char* FRAGMENT_SHADER_SOURCE = R"(
#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;

uniform sampler2D uTexture;

void main() {
    vec3 debugNormal = normalize(Normal) * 0.5 + 0.5;
    FragColor = texture(uTexture, TexCoord);
}
)";

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned vertex_shader = graphics_util::compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE);
    if (vertex_shader == 0) return -1;
    unsigned fragment_shader = graphics_util::compile_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE);
    if (fragment_shader == 0) return -1;

    unsigned shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    int succeed;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &succeed);
    if (!succeed) {
        char log_buf[512];
        glGetProgramInfoLog(shader_program, 512, nullptr, log_buf);
        std::cout << "Err: Shader program link failed: " << log_buf << std::endl;
        return -1;
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    /*
    unsigned VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(chr::Vertices::vertices),
        chr::Vertices::vertices,
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    */
    
    chr::Model model = chr::load_obj("assets/Sponza-master/sponza.obj");

    unsigned VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(chr::Model::Vertex),
        model.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int),
        model.indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(chr::Model::Vertex),
        (void*)offsetof(chr::Model::Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(chr::Model::Vertex),
        (void*)offsetof(chr::Model::Vertex, tex_coord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(chr::Model::Vertex),
        (void*)offsetof(chr::Model::Vertex, normal));
    glBindVertexArray(0);

    unsigned texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    {
        int tex_width, tex_height, tex_channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load("assets/texture.png",
            &tex_width, &tex_height, &tex_channels, 0);
        if (data != nullptr) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
        } else {
            std::cout << "Failed to load texture" << std::endl;
            return -1;
        }
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

        glUseProgram(shader_program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shader_program, "uTexture"), 0);

        glUseProgram(shader_program);

        glm::mat4 mat_projection = camera.get_projection_matrix(
            static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"),
            1, GL_FALSE, &mat_projection[0][0]);

        glm::mat4 mat_view = camera.get_view_matrix();
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"),
            1, GL_FALSE, &mat_view[0][0]);

        glBindVertexArray(VAO);

        glm::mat4 mat_model = glm::mat4(1.0f);
        mat_model = glm::scale(mat_model, glm::vec3(0.01f));
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"),
            1, GL_FALSE, &mat_model[0][0]);

        
        glDrawElements(GL_TRIANGLES, static_cast<unsigned>(model.indices.size()), GL_UNSIGNED_INT, nullptr);
        
        /*
        constexpr int cube_count = sizeof(chr::Vertices::cubePositions)
            / sizeof(chr::Vertices::cubePositions[0]);
        for (int i = 0; i < cube_count; ++i) {

            glm::mat4 mat_model = glm::mat4(1.0f);
            mat_model = glm::translate(mat_model, chr::Vertices::cubePositions[i]);
            float angle = 20.0f * i + 1.0f * frames;
            mat_model = glm::rotate(mat_model, glm::radians(angle),
                glm::vec3(1.0f, 0.3f, 0.5f));

            glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"),
                1, GL_FALSE, &mat_model[0][0]);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        */

        glfwSwapBuffers(window);
        glfwPollEvents();

        ++frames;
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(shader_program);

    glfwTerminate();
    return 0;
}

void process_input(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    float camera_speed = 0.5f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.position += camera_speed * camera.dir_front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.position -= camera_speed * camera.dir_front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.position -= glm::normalize(glm::cross(camera.dir_front, camera.dir_up)) * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.position += glm::normalize(glm::cross(camera.dir_front, camera.dir_up)) * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.position += camera_speed * camera.dir_up;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.position -= camera_speed * camera.dir_up;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}