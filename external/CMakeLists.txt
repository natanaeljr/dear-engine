cmake_minimum_required(VERSION 3.5)
include(ExternalProject)

#########################################################################################
# SPDLOG
#########################################################################################
# SPDLog is very fast, header-only or compiled, C++ logging library.
# SPDLog is licensed under the MIT license.
message(STATUS "Adding external library: SPDLOG")
ExternalProject_Add(spdlog_external
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/spdlog
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DSPDLOG_INSTALL=ON
        -DSPDLOG_BUILD_SHARED=OFF
        -DSPDLOG_BUILD_TESTS=OFF
        -DSPDLOG_BUILD_EXAMPLE=OFF
        -DSPDLOG_FMT_EXTERNAL=OFF
        -DSPDLOG_NO_EXCEPTIONS=ON)

#########################################################################################
# GLFW
#########################################################################################
# GLFW is multi-platform library for OpenGL, OpenGL ES and Vulkan development on desktop.
# Provides an API for creating windows, contexts and surfaces, receiving input and events.
# GLFW is licensed under the zlib/libpng license.
message(STATUS "Adding external library: GLFW")
ExternalProject_Add(glfw_external
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/glfw
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS=OFF
        -DGLFW_BUILD_DOCS=OFF
        -DGLFW_BUILD_TESTS=OFF
        -DGLFW_BUILD_EXAMPLES=OFF)

#########################################################################################
# glbinding
#########################################################################################
# glbinding is a cross-platform C++ binding for the OpenGL API.
# glbinding leverages C++11 features like enum classes, lambdas, and variadic templates,
# instead of relying on macros.
# glbinding is licensed under the MIT license.
message(STATUS "Adding external library: glbinding")
ExternalProject_Add(glbinding_external
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/glbinding
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glbinding
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS=OFF
        -DOPTION_BUILD_TOOLS=OFF
        -DOPTION_BUILD_EXAMPLES=OFF
        -DOPTION_SELF_CONTAINED=ON
        -DOPTION_BUILD_OWN_KHR_HEADERS=ON)

#########################################################################################
# GLM
#########################################################################################
# OpenGL Mathematics (GLM) is a header only C++ mathematics library for
# graphics software based on the OpenGL Shading Language (GLSL) specifications.
# GLM is licensed under The Happy Bunny License (Modified MIT License) or MIT License.
message(STATUS "Adding external library: GLM")
ExternalProject_Add(glm_external
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/glm
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_STATIC_LIBS=ON
        -DGLM_TEST_ENABLE=OFF)

#########################################################################################
# STB
#########################################################################################
# stb single-file public domain libraries for C/C++
# stb is licensed under the MIT license and Public Domain.
message(STATUS "Adding external library: STB")
ExternalProject_Add(stb_external
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/stb
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DSTB_IMAGE=ON
        -DSTB_TRUETYPE=ON
        -DSTB_RECT_PACK=ON)

#########################################################################################
# GSL
#########################################################################################
# The Guidelines Support Library (GSL) contains functions and types that are suggested
# for use by the C++ Core Guidelines maintained by the Standard C++ Foundation.
# This repo contains Microsoft's implementation of GSL.
# Microsoft GSL is licensed under the MIT License.
message(STATUS "Adding external library: GSL")
ExternalProject_Add(gsl_external
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gsl
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gsl
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        -DBUILD_SHARED_LIBS=OFF
        -DGSL_TEST=OFF)

#########################################################################################
# dr_libs
#########################################################################################
# Single file audio decoding libraries for C/C++.
# dr_libs is licensed under the Public Domain license and MIT license.
message(STATUS "Adding external library: dr_libs")
ExternalProject_Add(drlibs_external
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/drlibs
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/drlibs
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        -DBUILD_SHARED_LIBS=OFF
        -DDR_WAV=ON)

#########################################################################################
# OpenAL
#########################################################################################
# OpenAL Soft is a software implementation of the OpenAL 3D audio API.
# Provides capabilities for playing audio in a virtual 3D environment.
# OpenAL is licensed under the LGPL license.
message(STATUS "Adding external library: openal")
ExternalProject_Add(openal_external
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/openal
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/openal
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_CACHE_ARGS
        -DLIBTYPE:string=STATIC
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        -DBUILD_SHARED_LIBS=OFF
        -DALSOFT_EXAMPLES=OFF)

