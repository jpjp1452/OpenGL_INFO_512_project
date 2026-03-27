# OpenGL Standalone Project

A minimal standalone OpenGL project with camera control and basic shaders.

## Project Structure

```
project/
├── CMakeLists.txt       # Build configuration
├── main.cpp             # Main application with OpenGL setup
├── camera.h             # Camera class for 3D navigation
├── shader.h             # Shader compilation and management
└── shaders/
    ├── basic.vert       # Basic vertex shader
    └── basic.frag       # Basic fragment shader
```

## Features

- **OpenGL 3.3 Core Profile** setup with GLAD and GLFW
- **Camera System** with keyboard-based movement and rotation
- **Shader Management** via the Shader class
- **Simple Triangle** rendering example

## Dependencies

This project requires:
- **GLAD** - OpenGL loader
- **GLFW3** - Window and input management
- **GLM** - Math library for graphics
- **OpenGL 3.3+**

All dependencies are located in the `../3rdParty/` directory relative to this project folder.

## Building

```bash
cd project
mkdir build
cd build
cmake ..
make
```

The CMake build process will automatically copy the shader files to the build directory.

## Running

```bash
cd build
./OpenGLProject
```

**Important:** Run the executable from the `build` directory so it can find the shader files.

## Controls

- **W/A/S/D** - Move camera (forward/left/backward/right)
- **Arrow Keys** - Rotate camera view
- **ESC** - Exit application

## Default Scene

The application renders a simple triangle with:
- Red vertex (top-left)
- Green vertex (top-right) 
- Blue vertex (center)

You can navigate around it using the camera controls.

## Notes

- The shader files are loaded from the `shaders/` directory relative to the executable
- Ensure you run the executable from the build directory or adjust the working directory accordingly
- To modify the geometry, edit the `vertices[]` array in `main.cpp`
- To use different shaders, modify the shader loading call in `main.cpp` or add support for multiple shader programs
