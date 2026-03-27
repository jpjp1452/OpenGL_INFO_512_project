#Template from INFOH502 which is heavily inspired from https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt

cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
#Project setup
project(OpenGLProject)

#Set minimal version of cmake and c++ standard
set(CMAKE_CXX_STANDARD 14)     
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenGL REQUIRED)

#Glad library
add_library(glad STATIC 3rdParty/glad/src/glad.c)
set(GLAD_INCLUDE "3rdParty/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

#GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

#Search for glfw in case it is installed
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    add_subdirectory(3rdParty/glfw)
    include_directories(3rdParty/glfw/include/)
endif()

include_directories(3rdParty/glad/include/
                    3rdParty/glm/
                    3rdParty/stb/)

# Add your project subfolder
add_subdirectory(project)
