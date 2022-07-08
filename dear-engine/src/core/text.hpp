#pragma once

#include <string_view>

#include <glbinding/gl33core/types.h>
using namespace gl;

#include "gl_font.hpp"
#include "gl_object.hpp"
#include "gl_shader.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Text

/// Generate quad vertices for a text with the given font.
auto gen_text_quads(const GLFont& font, std::string_view text) -> std::tuple<std::vector<TextureVertex>, std::vector<GLushort>, float>;

/// Update GLObject data with the new generated quads for the given Text
void update_text_globject(const GLShader& shader, GLObject& glo, const GLFont& font, std::string_view text, GLenum usage = GL_STATIC_DRAW);

/// Upload new Text Indexed-Vertex object to GPU memory
auto create_text_globject(const GLShader& shader, gsl::span<const TextureVertex> vertices, gsl::span<const GLushort> indices, GLenum usage) -> GLObject;

