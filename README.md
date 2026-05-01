# projOpenGL

C++ OpenGL renderer project built with CMake. The current app opens a GLFW window, initializes GLAD, loads the Sponza OBJ scene, uploads mesh data to the GPU, and renders it with a simple textured shader.

## Current state

- Language: C++17
- Build system: CMake
- Window/context: GLFW
- OpenGL loader: GLAD
- Math: GLM
- Image loading: `stb_image`
- Model import: Assimp
- Sample asset: Sponza scene from a local `assets/Sponza-master/` directory

## External libraries

The project is set up to use third-party code from `external/` in the local working tree.

- `external/assimp`: imports mesh and material data from model files. The current renderer uses it to load the Sponza OBJ scene into the project's raw scene structures.
- `external/glad`: loads OpenGL function pointers at runtime so the application can call modern OpenGL APIs after creating a context.
- `external/glfw`: creates the window and OpenGL context, and handles input and the main event loop.
- `external/glm`: provides vector and matrix math used for transforms, camera state, and projection/view calculations.
- `external/stb_image.h` and `external/stb_image.cpp`: load texture image files into CPU memory before uploading them to OpenGL textures.

In this repo configuration, these dependencies are expected to exist under `external/` locally, but they are not intended to be tracked by git here. A fresh clone may require those directories or files to be provided separately.

## Assets

The renderer expects local assets under `assets/`.

- `assets/Sponza-master/`: Sponza OBJ scene and textures used as the current test scene
- `assets/texture.png`: texture used by the simple shader path

Like the local dependency directories above, the Sponza scene directory is expected in the working tree but is not intended to be tracked by git in this repo configuration.

## Build

This project is currently configured for Windows linking through `opengl32`.

```powershell
cmake -S . -B out/build
cmake --build out/build --config Debug
```

If you use Visual Studio, CMake may also generate additional files under `.vs/` and `out/`.
