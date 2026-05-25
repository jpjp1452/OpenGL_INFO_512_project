# 3DGRAPHICS_INFO_512_project: Space Game

## Project Structure

```text
CMakeLists.txt          # Root build configuration
README.md               # Root README
3rdParty/               # External dependencies (GLAD, GLFW, GLM, stb)
build/                  # Build directory containing executables
project/                # Main project source directory
├── CMakeLists.txt      # Project build configuration
├── README.md           # Project documentation
├── main.cpp            # Main application entry point and OpenGL setup
├── camera.h            # Camera class for 3D navigation
├── shader.h            # Shader compilation and management
├── alienManager.hpp    # Alien entity management
├── LSystem.hpp         # L-System procedural generation functionality
├── objectManager.h     # Scene object management
├── terrainManager.h    # Terrain generation and handling
├── Model/              # 3D models and materials (.mtl)
├── Textures/           # Textures including cubemaps
└── shaders/            # Vertex, fragment, geometry, and tessellation shaders
```
## Dependencies

This project requires:
- **GLAD** - OpenGL loader
- **GLFW3** - Window and input management
- **GLM** - Math library for graphics
- **OpenGL 3.3+**

All dependencies are located in the `../3rdParty/` directory relative to this project folder.


## Installation:
- Ensure that 3rdParty is the same current directory as the folder project and the CMakeLists.txt
  
# Building

```bash
mkdir build
cd build
cmake ..
make
```

## Launch

```bash
cd build/project
./OpenGLProject
```

## Controls

- **Z/Q/S/D** - Move camera (forward/left/backward/right)
- **Mouse** - Rotate camera view
- **Left Click** - Shooting with pistol
- **ESC** - Exit application