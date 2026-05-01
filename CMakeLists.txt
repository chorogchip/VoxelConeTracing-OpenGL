cmake_minimum_required(VERSION 3.14)
project(OpenGLTest)

set(CMAKE_CXX_STANDARD 17)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
add_subdirectory(external/assimp)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

add_executable(${PROJECT_NAME} 
    ${SOURCES} 
    external/glad/src/glad.c
    external/stb_image.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE 
    external/
    external/glfw/include
    external/glad/include
    external/assimp/include
    ${CMAKE_BINARY_DIR}/external/assimp/include
)

target_link_libraries(${PROJECT_NAME} glfw assimp opengl32)
