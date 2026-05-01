#pragma once

#include <cstdint>
#include <string>

namespace graphics_util {
	unsigned compile_shader(unsigned type, const char* source);
	unsigned compile_shader_from_file(unsigned type, const std::string& path);
	uint32_t load_texture_2d(const std::string& filename);
}
