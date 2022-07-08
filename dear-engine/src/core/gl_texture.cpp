#include "./gl_texture.hpp"

#include <memory>
#include <string>
#include <optional>

#include <glbinding/gl33core/gl.h>
using namespace gl;

#include <stb/stb_image.h>

#include "log.hpp"
#include "file.hpp"
#include "unique_num.hpp"

using namespace std::string_literals;

/// Read file and upload RGB/RBGA texture to GPU memory
auto load_rgba_texture(const std::string& inpath, GLenum min_filter, GLenum mag_filter) -> std::optional<GLTexture>
{
  const std::string filepath = ENGINE_ASSETS_PATH + "/"s + inpath;
  auto file = read_file_to_string(filepath);
  if (!file) { ERROR("Failed to read texture path ({})", filepath); return std::nullopt; }
  int width, height, channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load_from_memory((const uint8_t*)file->data(), file->length(), &width, &height, &channels, 0);
  if (!data) { ERROR("Failed to load texture path ({})", filepath); return std::nullopt; }
  ASSERT_MSG(channels == 4 || channels == 3, "actual channels: {}", channels);
  GLenum type = (channels == 4) ? GL_RGBA : GL_RGB;
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter != GLenum(0) ? mag_filter : min_filter);
  glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data);
  return GLTexture{ texture };
}

/// Upload font bitmap texture to GPU memory
auto load_font_texture(const uint8_t data[], size_t width, size_t height) -> GLTexture
{
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture); 
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  return { texture };
}

