#include "g_buffer_resources.h"

#include <array>
#include <cstdio>
#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics_util.h"

namespace {
    constexpr const char* LIGHTING_VERTEX_SHADER_PATH = "assets/shaders/deferred_light.vert";
    constexpr const char* LIGHTING_FRAGMENT_SHADER_PATH = "assets/shaders/deferred_light.frag";
    constexpr const char* DEBUG_FRAGMENT_SHADER_PATH = "assets/shaders/debug_buffer.frag";
    constexpr const char* LIGHT_MARKER_VERTEX_SHADER_PATH = "assets/shaders/light_marker.vert";
    constexpr const char* LIGHT_MARKER_FRAGMENT_SHADER_PATH = "assets/shaders/light_marker.frag";
    const glm::vec3 LIGHT_DIRECTION = glm::normalize(glm::vec3(-0.6f, -1.0f, -0.35f));
    const glm::vec3 LIGHT_COLOR = glm::vec3(1.0f, 0.98f, 0.92f);
    constexpr int SHADOW_MAP_SIZE = 2048;
    constexpr float SHADOW_ORTHO_HALF_EXTENT = 18.0f;
    constexpr float SHADOW_NEAR_PLANE = 1.0f;
    constexpr float SHADOW_FAR_PLANE = 60.0f;
    constexpr float SHADOW_LIGHT_DISTANCE = 24.0f;
    const glm::vec3 SHADOW_TARGET = glm::vec3(0.0f, 6.0f, 0.0f);
    constexpr float AMBIENT_STRENGTH = 0.32f;
    constexpr float DIFFUSE_STRENGTH = 0.85f;
    constexpr int POINT_LIGHT_COUNT = 5;
    constexpr float LIGHT_MARKER_SCALE = 0.18f;

    struct PointLightDesc {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float range;
    };

    const std::array<PointLightDesc, POINT_LIGHT_COUNT> POINT_LIGHTS = {{
        {
            glm::vec3(0.0f, 5.5f, 0.0f),
            glm::vec3(1.0f, 0.82f, 0.55f),
            12.0f,
            3.0f
        },
        {
            glm::vec3(-5.5f, 3.0f, 4.0f),
            glm::vec3(1.0f, 0.55f, 0.35f),
            8.0f,
            2.5f
        },
        {
            glm::vec3(5.5f, 3.0f, 4.0f),
            glm::vec3(0.45f, 0.65f, 1.0f),
            8.0f,
            2.5f
        },
        {
            glm::vec3(-5.5f, 3.0f, -4.0f),
            glm::vec3(0.55f, 1.0f, 0.65f),
            8.0f,
            2.5f
        },
        {
            glm::vec3(5.5f, 3.0f, -4.0f),
            glm::vec3(1.0f, 0.35f, 0.45f),
            8.0f,
            2.5f
        }
    }};

    glm::mat4 get_directional_light_view_projection_matrix() {
        const glm::vec3 light_position = SHADOW_TARGET - LIGHT_DIRECTION * SHADOW_LIGHT_DISTANCE;
        const glm::mat4 light_view = glm::lookAt(light_position, SHADOW_TARGET, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 light_projection = glm::ortho(
            -SHADOW_ORTHO_HALF_EXTENT, SHADOW_ORTHO_HALF_EXTENT,
            -SHADOW_ORTHO_HALF_EXTENT, SHADOW_ORTHO_HALF_EXTENT,
            SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
        return light_projection * light_view;
    }

    int create_g_buffer_attachments(chr::GBufferResources* resources, int width, int height) {
        glGenFramebuffers(1, &resources->framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, resources->framebuffer);

        glGenTextures(1, &resources->texture_albedo);
        glBindTexture(GL_TEXTURE_2D, resources->texture_albedo);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8,
            width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resources->texture_albedo, 0);

        glGenTextures(1, &resources->texture_normal);
        glBindTexture(GL_TEXTURE_2D, resources->texture_normal);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16F,
            width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, resources->texture_normal, 0);

        glGenTextures(1, &resources->texture_depth);
        glBindTexture(GL_TEXTURE_2D, resources->texture_depth);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, resources->texture_depth, 0);

        constexpr GLenum draw_buffers[] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1
        };
        glDrawBuffers(2, draw_buffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: G-buffer framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        resources->width = width;
        resources->height = height;
        return 0;
    }

    int create_shadow_attachments(chr::GBufferResources* resources) {
        glGenFramebuffers(1, &resources->shadow_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, resources->shadow_framebuffer);

        glGenTextures(1, &resources->shadow_texture_depth);
        glBindTexture(GL_TEXTURE_2D, resources->shadow_texture_depth);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        constexpr float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, resources->shadow_texture_depth, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: Shadow framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return 0;
    }

    void clear_g_buffer_attachments(chr::GBufferResources* resources) {
        if (resources->texture_depth != 0) {
            glDeleteTextures(1, &resources->texture_depth);
            resources->texture_depth = 0;
        }

        if (resources->texture_normal != 0) {
            glDeleteTextures(1, &resources->texture_normal);
            resources->texture_normal = 0;
        }

        if (resources->texture_albedo != 0) {
            glDeleteTextures(1, &resources->texture_albedo);
            resources->texture_albedo = 0;
        }

        if (resources->framebuffer != 0) {
            glDeleteFramebuffers(1, &resources->framebuffer);
            resources->framebuffer = 0;
        }

        resources->width = 0;
        resources->height = 0;
    }

    void clear_shadow_attachments(chr::GBufferResources* resources) {
        if (resources->shadow_texture_depth != 0) {
            glDeleteTextures(1, &resources->shadow_texture_depth);
            resources->shadow_texture_depth = 0;
        }

        if (resources->shadow_framebuffer != 0) {
            glDeleteFramebuffers(1, &resources->shadow_framebuffer);
            resources->shadow_framebuffer = 0;
        }
    }
}

namespace chr {

    int GBufferResources::init(int width, int height) {
        clear();

        if (width <= 0 || height <= 0) {
            std::cout << "Err: Invalid G-buffer size." << std::endl;
            return -1;
        }

        if (create_g_buffer_attachments(this, width, height) != 0) {
            clear();
            return -1;
        }
        if (create_shadow_attachments(this) != 0) {
            clear();
            return -1;
        }

        constexpr float quad_vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &this->quad_vao);
        glGenBuffers(1, &this->quad_vbo);
        glBindVertexArray(this->quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);

        constexpr float tetra_vertices[] = {
             0.0f,  1.0f,  0.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,

             0.0f,  1.0f,  0.0f,
             1.0f, -1.0f,  1.0f,
             0.0f, -1.0f, -1.0f,

             0.0f,  1.0f,  0.0f,
             0.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f, -1.0f,  1.0f,
             0.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &this->light_marker_vao);
        glGenBuffers(1, &this->light_marker_vbo);
        glBindVertexArray(this->light_marker_vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->light_marker_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tetra_vertices), tetra_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);

        const unsigned vertex_shader = graphics_util::compile_shader_from_file(
            GL_VERTEX_SHADER, LIGHTING_VERTEX_SHADER_PATH);
        if (vertex_shader == 0) {
            clear();
            return -1;
        }
        const unsigned fragment_shader = graphics_util::compile_shader_from_file(
            GL_FRAGMENT_SHADER, LIGHTING_FRAGMENT_SHADER_PATH);
        if (fragment_shader == 0) {
            glDeleteShader(vertex_shader);
            clear();
            return -1;
        }

        this->lighting_shader_program = glCreateProgram();
        glAttachShader(this->lighting_shader_program, vertex_shader);
        glAttachShader(this->lighting_shader_program, fragment_shader);
        glLinkProgram(this->lighting_shader_program);

        int succeed = 0;
        glGetProgramiv(this->lighting_shader_program, GL_LINK_STATUS, &succeed);
        if (!succeed) {
            char log_buf[512];
            glGetProgramInfoLog(this->lighting_shader_program, 512, nullptr, log_buf);
            std::cout << "Err: Deferred light shader link failed: " << log_buf << std::endl;
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            clear();
            return -1;
        }
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        this->uniform_g_albedo = glGetUniformLocation(this->lighting_shader_program, "gAlbedo");
        this->uniform_g_normal = glGetUniformLocation(this->lighting_shader_program, "gNormal");
        this->uniform_g_depth = glGetUniformLocation(this->lighting_shader_program, "gDepth");
        this->uniform_shadow_map = glGetUniformLocation(this->lighting_shader_program, "uShadowMap");
        this->uniform_inverse_projection = glGetUniformLocation(this->lighting_shader_program, "uInverseProjection");
        this->uniform_inverse_view = glGetUniformLocation(this->lighting_shader_program, "uInverseView");
        this->uniform_light_view_projection = glGetUniformLocation(this->lighting_shader_program, "uLightViewProjection");
        this->uniform_light_direction = glGetUniformLocation(this->lighting_shader_program, "uLightDirection");
        this->uniform_light_color = glGetUniformLocation(this->lighting_shader_program, "uLightColor");
        this->uniform_ambient_strength = glGetUniformLocation(this->lighting_shader_program, "uAmbientStrength");
        this->uniform_diffuse_strength = glGetUniformLocation(this->lighting_shader_program, "uDiffuseStrength");
        this->uniform_point_light_count = glGetUniformLocation(this->lighting_shader_program, "uPointLightCount");
        for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
            char uniform_name[64];
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightPositions[%d]", i);
            this->uniform_point_light_positions[i] = glGetUniformLocation(this->lighting_shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightColors[%d]", i);
            this->uniform_point_light_colors[i] = glGetUniformLocation(this->lighting_shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightIntensities[%d]", i);
            this->uniform_point_light_intensities[i] = glGetUniformLocation(this->lighting_shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightRanges[%d]", i);
            this->uniform_point_light_ranges[i] = glGetUniformLocation(this->lighting_shader_program, uniform_name);
        }

        const unsigned debug_vertex_shader = graphics_util::compile_shader_from_file(
            GL_VERTEX_SHADER, LIGHTING_VERTEX_SHADER_PATH);
        if (debug_vertex_shader == 0) {
            clear();
            return -1;
        }
        const unsigned debug_fragment_shader = graphics_util::compile_shader_from_file(
            GL_FRAGMENT_SHADER, DEBUG_FRAGMENT_SHADER_PATH);
        if (debug_fragment_shader == 0) {
            glDeleteShader(debug_vertex_shader);
            clear();
            return -1;
        }

        this->debug_shader_program = glCreateProgram();
        glAttachShader(this->debug_shader_program, debug_vertex_shader);
        glAttachShader(this->debug_shader_program, debug_fragment_shader);
        glLinkProgram(this->debug_shader_program);

        succeed = 0;
        glGetProgramiv(this->debug_shader_program, GL_LINK_STATUS, &succeed);
        if (!succeed) {
            char log_buf[512];
            glGetProgramInfoLog(this->debug_shader_program, 512, nullptr, log_buf);
            std::cout << "Err: Debug buffer shader link failed: " << log_buf << std::endl;
            glDeleteShader(debug_vertex_shader);
            glDeleteShader(debug_fragment_shader);
            clear();
            return -1;
        }
        glDeleteShader(debug_vertex_shader);
        glDeleteShader(debug_fragment_shader);

        this->uniform_debug_texture = glGetUniformLocation(this->debug_shader_program, "uTexture");
        this->uniform_debug_mode = glGetUniformLocation(this->debug_shader_program, "uMode");

        const unsigned light_marker_vertex_shader = graphics_util::compile_shader_from_file(
            GL_VERTEX_SHADER, LIGHT_MARKER_VERTEX_SHADER_PATH);
        if (light_marker_vertex_shader == 0) {
            clear();
            return -1;
        }
        const unsigned light_marker_fragment_shader = graphics_util::compile_shader_from_file(
            GL_FRAGMENT_SHADER, LIGHT_MARKER_FRAGMENT_SHADER_PATH);
        if (light_marker_fragment_shader == 0) {
            glDeleteShader(light_marker_vertex_shader);
            clear();
            return -1;
        }

        this->light_marker_shader_program = glCreateProgram();
        glAttachShader(this->light_marker_shader_program, light_marker_vertex_shader);
        glAttachShader(this->light_marker_shader_program, light_marker_fragment_shader);
        glLinkProgram(this->light_marker_shader_program);

        succeed = 0;
        glGetProgramiv(this->light_marker_shader_program, GL_LINK_STATUS, &succeed);
        if (!succeed) {
            char log_buf[512];
            glGetProgramInfoLog(this->light_marker_shader_program, 512, nullptr, log_buf);
            std::cout << "Err: Light marker shader link failed: " << log_buf << std::endl;
            glDeleteShader(light_marker_vertex_shader);
            glDeleteShader(light_marker_fragment_shader);
            clear();
            return -1;
        }
        glDeleteShader(light_marker_vertex_shader);
        glDeleteShader(light_marker_fragment_shader);

        this->uniform_marker_model = glGetUniformLocation(this->light_marker_shader_program, "model");
        this->uniform_marker_view = glGetUniformLocation(this->light_marker_shader_program, "view");
        this->uniform_marker_projection = glGetUniformLocation(this->light_marker_shader_program, "projection");
        this->uniform_marker_color = glGetUniformLocation(this->light_marker_shader_program, "uColor");
        return 0;
    }

    int GBufferResources::resize(int width, int height) {
        if (width <= 0 || height <= 0) {
            return -1;
        }

        if (this->width == width && this->height == height) {
            return 0;
        }

        clear_g_buffer_attachments(this);
        if (create_g_buffer_attachments(this, width, height) != 0) {
            clear_g_buffer_attachments(this);
            return -1;
        }

        return 0;
    }

    void GBufferResources::clear() {
        if (this->debug_shader_program != 0) {
            glDeleteProgram(this->debug_shader_program);
            this->debug_shader_program = 0;
        }

        if (this->light_marker_shader_program != 0) {
            glDeleteProgram(this->light_marker_shader_program);
            this->light_marker_shader_program = 0;
        }

        if (this->lighting_shader_program != 0) {
            glDeleteProgram(this->lighting_shader_program);
            this->lighting_shader_program = 0;
        }

        if (this->quad_vbo != 0) {
            glDeleteBuffers(1, &this->quad_vbo);
            this->quad_vbo = 0;
        }

        if (this->quad_vao != 0) {
            glDeleteVertexArrays(1, &this->quad_vao);
            this->quad_vao = 0;
        }

        if (this->light_marker_vbo != 0) {
            glDeleteBuffers(1, &this->light_marker_vbo);
            this->light_marker_vbo = 0;
        }

        if (this->light_marker_vao != 0) {
            glDeleteVertexArrays(1, &this->light_marker_vao);
            this->light_marker_vao = 0;
        }

        clear_g_buffer_attachments(this);
        clear_shadow_attachments(this);
        this->uniform_shadow_map = -1;
        this->uniform_g_albedo = -1;
        this->uniform_g_normal = -1;
        this->uniform_g_depth = -1;
        this->uniform_inverse_projection = -1;
        this->uniform_inverse_view = -1;
        this->uniform_light_view_projection = -1;
        this->uniform_light_direction = -1;
        this->uniform_light_color = -1;
        this->uniform_ambient_strength = -1;
        this->uniform_diffuse_strength = -1;
        this->uniform_point_light_count = -1;
        this->uniform_point_light_positions.fill(-1);
        this->uniform_point_light_colors.fill(-1);
        this->uniform_point_light_intensities.fill(-1);
        this->uniform_point_light_ranges.fill(-1);
        this->uniform_marker_model = -1;
        this->uniform_marker_view = -1;
        this->uniform_marker_projection = -1;
        this->uniform_marker_color = -1;
        this->uniform_debug_texture = -1;
        this->uniform_debug_mode = -1;
    }

    void GBufferResources::bind_for_geometry_pass() {
        glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);
        glViewport(0, 0, this->width, this->height);
    }

    glm::mat4 GBufferResources::get_directional_light_view_projection() const {
        return get_directional_light_view_projection_matrix();
    }

    void GBufferResources::bind_for_shadow_pass() {
        glBindFramebuffer(GL_FRAMEBUFFER, this->shadow_framebuffer);
        glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    }

    void GBufferResources::draw_lighting_pass(const glm::mat4& mat_projection, const glm::mat4& mat_view) {
        bind_default_framebuffer(this->width, this->height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 inverse_projection = glm::inverse(mat_projection);
        const glm::mat4 inverse_view = glm::inverse(mat_view);
        const glm::mat4 light_view_projection = get_directional_light_view_projection_matrix();
        const glm::vec3 view_light_direction =
            glm::normalize(glm::mat3(mat_view) * LIGHT_DIRECTION);

        glUseProgram(this->lighting_shader_program);
        glUniform1i(this->uniform_g_albedo, 0);
        glUniform1i(this->uniform_g_normal, 1);
        glUniform1i(this->uniform_g_depth, 2);
        glUniform1i(this->uniform_shadow_map, 3);
        glUniformMatrix4fv(this->uniform_inverse_projection, 1, GL_FALSE, &inverse_projection[0][0]);
        glUniformMatrix4fv(this->uniform_inverse_view, 1, GL_FALSE, &inverse_view[0][0]);
        glUniformMatrix4fv(this->uniform_light_view_projection, 1, GL_FALSE, &light_view_projection[0][0]);
        glUniform3fv(this->uniform_light_direction, 1, &view_light_direction[0]);
        glUniform3fv(this->uniform_light_color, 1, &LIGHT_COLOR[0]);
        glUniform1f(this->uniform_ambient_strength, AMBIENT_STRENGTH);
        glUniform1f(this->uniform_diffuse_strength, DIFFUSE_STRENGTH);
        glUniform1i(this->uniform_point_light_count, POINT_LIGHT_COUNT);

        for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
            const glm::vec3 view_light_position = glm::vec3(mat_view * glm::vec4(POINT_LIGHTS[i].position, 1.0f));
            glUniform3fv(this->uniform_point_light_positions[i], 1, &view_light_position[0]);
            glUniform3fv(this->uniform_point_light_colors[i], 1, &POINT_LIGHTS[i].color[0]);
            glUniform1f(this->uniform_point_light_intensities[i], POINT_LIGHTS[i].intensity);
            glUniform1f(this->uniform_point_light_ranges[i], POINT_LIGHTS[i].range);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->texture_albedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->texture_normal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, this->texture_depth);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, this->shadow_texture_depth);

        glBindVertexArray(this->quad_vao);
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
    }

    void GBufferResources::draw_light_markers(const glm::mat4& mat_projection, const glm::mat4& mat_view) {
        bind_default_framebuffer(this->width, this->height);

        glUseProgram(this->light_marker_shader_program);
        glUniformMatrix4fv(this->uniform_marker_view, 1, GL_FALSE, &mat_view[0][0]);
        glUniformMatrix4fv(this->uniform_marker_projection, 1, GL_FALSE, &mat_projection[0][0]);
        glBindVertexArray(this->light_marker_vao);
        glDisable(GL_DEPTH_TEST);

        for (const auto& point_light : POINT_LIGHTS) {
            glm::mat4 mat_model = glm::translate(glm::mat4(1.0f), point_light.position);
            mat_model = glm::scale(mat_model, glm::vec3(LIGHT_MARKER_SCALE));
            glUniformMatrix4fv(this->uniform_marker_model, 1, GL_FALSE, &mat_model[0][0]);
            glUniform3fv(this->uniform_marker_color, 1, &point_light.color[0]);
            glDrawArrays(GL_TRIANGLES, 0, 12);
        }

        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
    }

    void GBufferResources::draw_debug_views() {
        constexpr int preview_count = 3;
        const int padding = 16;
        const int preview_width = this->width / 5;
        const int preview_height = this->height / 5;
        const int x = this->width - preview_width - padding;

        const uint32_t preview_textures[preview_count] = {
            this->texture_albedo,
            this->texture_normal,
            this->texture_depth
        };
        const int preview_modes[preview_count] = { 0, 1, 2 };

        glUseProgram(this->debug_shader_program);
        glUniform1i(this->uniform_debug_texture, 0);
        glBindVertexArray(this->quad_vao);
        glDisable(GL_DEPTH_TEST);

        for (int i = 0; i < preview_count; ++i) {
            const int y = this->height - padding - ((i + 1) * preview_height);
            glViewport(x, y, preview_width, preview_height);
            glUniform1i(this->uniform_debug_mode, preview_modes[i]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, preview_textures[i]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
        glViewport(0, 0, this->width, this->height);
    }

    void GBufferResources::bind_default_framebuffer(int width, int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
    }

}
