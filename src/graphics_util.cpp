#include "graphics_util.h"

#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <stb_image.h>


namespace graphics_util {

	unsigned compile_shader(unsigned type, const char* source) {
		unsigned shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		int succeed;
		char log_buf[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &succeed);
		if (!succeed) {
			glGetShaderInfoLog(shader, 512, nullptr, log_buf);
			std::cout << "Err: Shader compile failed: " << log_buf << std::endl;
			return 0;
		}

		return shader;
	}

	unsigned compile_shader_from_file(unsigned type, const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cout << "Err: Failed to open shader file: " << path << std::endl;
			return 0;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		const std::string source = buffer.str();
		return compile_shader(type, source.c_str());
	}

	uint32_t load_texture_2d(const std::string& filename) {
		if (filename.empty()) {
			return 0;
		}

		int tex_width = 0;
		int tex_height = 0;
		int tex_channels = 0;

		stbi_set_flip_vertically_on_load(false);
		unsigned char* data = stbi_load(
			filename.c_str(),
			&tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
		if (data == nullptr) {
			std::cout << "Failed to load texture: " << filename << std::endl;
			return 0;
		}

		uint32_t texture = 0;
		GLint prev_unpack_alignment = 4;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack_alignment);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA8,
			tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack_alignment);
		stbi_image_free(data);

		return texture;
	}

}
