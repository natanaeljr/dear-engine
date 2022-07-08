#pragma once

#include "core/gl_shader.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Shaders

/// Holds the shaders used by the game
struct Shaders {
  GLShader generic_shader;
};

/// Loads all shaders used by the game
Shaders load_shaders();

/// Load Generic Shader
/// (supports rendering: Colored objects, Textured objects and BitmapFont text)
GLShader load_generic_shader();

