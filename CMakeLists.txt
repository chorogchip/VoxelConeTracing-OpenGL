cmake_minimum_required(VERSION 3.20)
project(OpenGLTest LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/CMakeLists.txt")
    add_subdirectory(external/glfw)
else()
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(glfw)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/assimp/CMakeLists.txt")
    add_subdirectory(external/assimp)
else()
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.4
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(assimp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glm/CMakeLists.txt")
    add_subdirectory(external/glm)
else()
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.3
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(glm)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include/glad/glad.h" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c")
    add_library(glad STATIC external/glad/src/glad.c)
    add_library(glad::glad ALIAS glad)
    target_include_directories(glad PUBLIC external/glad/include)
else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v0.1.36
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_GetProperties(glad)
    if(NOT glad_POPULATED)
        FetchContent_Populate(glad)
    endif()
    add_subdirectory(${glad_SOURCE_DIR}/cmake ${glad_BINARY_DIR}/cmake)
    glad_add_library(glad REPRODUCIBLE LOADER API gl:compatibility=4.6)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image.h")
    set(STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
else()
    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_GetProperties(stb)
    if(NOT stb_POPULATED)
        FetchContent_Populate(stb)
    endif()
    set(STB_INCLUDE_DIR "${stb_SOURCE_DIR}")
endif()

find_package(OpenGL REQUIRED)

file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${STB_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    glad::glad
    glm::glm
    assimp
)
