#include "graphics_util.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>


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

}