cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(OpenGLProjectExecutable)

# Include directories specific to this subproject
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add executable
add_executable(OpenGLProject 
               main.cpp 
               camera.h 
               shader.h 
               objectManager.h 
               objectTexture.h )

target_compile_definitions(OpenGLProject PRIVATE
    PATH_TO_OBJECTS="${CMAKE_CURRENT_SOURCE_DIR}/Model"
    PATH_TO_TEXTURE="${CMAKE_CURRENT_SOURCE_DIR}/Textures"
    PATH_TO_SHADERS="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

# Link libraries (OpenGL, glfw, glad are provided by the parent CMakeLists)
target_link_libraries(OpenGLProject PUBLIC OpenGL::GL glfw glad)

# Set working directory for shader path resolution
set_target_properties(OpenGLProject PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# Copy shaders to build directory after build
add_custom_command(TARGET OpenGLProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMENT "Copying shader files to build directory"
)
