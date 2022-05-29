#pragma once

#include <memory>
#include <string>
#include <optional>

#include <glbinding/gl33core/gl.h>
using namespace gl;

#include "./unique_num.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Texture

/// Represents a texture loaded to GPU memory
struct GLTexture {
  UniqueNum<GLuint> id;

  ~GLTexture() {
    if (id) glDeleteTextures(1, &id.inner);
  }

  // Movable but not Copyable
  GLTexture(GLTexture&&) = default;
  GLTexture(const GLTexture&) = delete;
  GLTexture& operator=(GLTexture&&) = default;
  GLTexture& operator=(const GLTexture&) = delete;
};

/// GLTexture reference type alias
using GLTextureRef = std::shared_ptr<GLTexture>;

/// Read file and upload RGB/RBGA texture to GPU memory
auto load_rgba_texture(const std::string& inpath, GLenum min_filter, GLenum mag_filter = GLenum(0)) -> std::optional<GLTexture>;

/// Upload font bitmap texture to GPU memory
auto load_font_texture(const uint8_t data[], size_t width, size_t height) -> GLTexture;

