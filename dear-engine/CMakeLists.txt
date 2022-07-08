cmake_minimum_required(VERSION 3.5)
project(dearengine LANGUAGES CXX)

# This project can be built as a standalone project on a system with the required
# libraries already installed, or it can be part of the Super-Project which builds all
# dependencies with the correct versions.
# For building it separately with the dependency builds from the Super-Project,
# the variable `CMAKE_PREFIX_PATH` must be set to the Super-Project's build/external dir.
# In Linux you can do it as:
#  cmake .. -DCMAKE_PREFIX_PATH=$(realpath ../../build/external)

#########################################################################################
# Configuration
#########################################################################################
# Default build type to Debug, for release use `cmake -DCMAKE_BUILD_TYPE=Release ..`
if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type default to Debug" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Turn on when having trouble building
set(CMAKE_VERBOSE_MAKEFILE OFF)
# Compilation Database for C++ language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# For OpenGL prefer new GL Vendor Neutral Dispatch (GLVND) rather than legacy
set(OpenGL_GL_PREFERENCE "GLVND")

# Default compilation settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -pg -fsanitize=undefined -Werror -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -Werror -Wall")

#########################################################################################
# Dependencies
#########################################################################################
# OpenGL for graphics library
find_package(OpenGL REQUIRED)
# OpenAL for audio library
find_package(OpenAL REQUIRED CONFIG NO_SYSTEM_ENVIRONMENT_PATH)
# SPDLog for fast C++ logging library
find_package(spdlog 1.10.0 EXACT REQUIRED)
# GLFW for desktop windowing and input
find_package(glfw3 3.4.0 EXACT REQUIRED)
# GLBinding for C++ binding of the OpenGL API
find_package(glbinding REQUIRED)
# GLM for opengl mathematics library
find_package(glm 0.9.9.9 EXACT REQUIRED)
# STB for image/font loading/decoding
find_package(stb 0.9.0 EXACT REQUIRED)
# GSL for types and functions of C++ Core Guidelines
find_package(Microsoft.GSL 4.0.0 EXACT REQUIRED)
# drlibs for audio decoding library
find_package(drlibs 0.10.0 EXACT REQUIRED)
# ImGui for graphical user interface
find_package(imgui 1.88 EXACT REQUIRED)
# yaml-cpp for YAML parser and emitter in C++
find_package(yaml-cpp 0.7.0 EXACT REQUIRED)

#########################################################################################
# Project
#########################################################################################
# Main target binary
add_executable(dearengine)
target_sources(dearengine PRIVATE
    src/main.cpp
    src/shaders.cpp
    src/core/renderer.cpp
    src/core/camera.cpp
    src/fonts.cpp
    src/core/file.cpp
    src/core/text.cpp
    src/core/sprite.cpp
    src/core/gl_font.cpp
    src/core/al_buffer.cpp
    src/core/al_source.cpp
    src/core/aabb.cpp
    src/core/gl_shader.cpp
    src/core/gl_object.cpp
    src/core/gl_texture.cpp
)
target_link_libraries(dearengine PRIVATE
    glm::glm
    spdlog::spdlog
    stb::stb_image
    stb::stb_truetype
    stb::stb_rect_pack
    yaml-cpp
    drlibs::dr_wav
    Microsoft.GSL::GSL
    imgui::imgui
    imgui::imgui_glfw
    imgui::imgui_opengl
    glfw
    glbinding::glbinding
    glbinding::glbinding-aux
    OpenGL::GL
    OpenAL::OpenAL
)
target_compile_definitions(dearengine PRIVATE
    AL_LIBTYPE_STATIC
    GLFW_INCLUDE_NONE
    SPDLOG_ACTIVE_LEVEL=$<IF:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>,SPDLOG_LEVEL_TRACE,SPDLOG_LEVEL_INFO>
    ENGINE_ASSETS_PATH=$<IF:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>,"${CMAKE_CURRENT_SOURCE_DIR}/assets","TODO">
)

install(TARGETS dearengine)
