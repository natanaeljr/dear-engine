#pragma once

#include <array>
#include <string>
#include <optional>

#include <glbinding/gl33core/types.h>
using namespace gl;

#include "unique_num.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Shader

/// Enumeration of supported Shader Attributes
enum class GLAttr {
  POSITION,
  COLOR,
  MODEL,
  TEXCOORD,
  COUNT, // must be last
};

/// Enumeration of supported Shader Uniforms
enum class GLUnif {
  COLOR,
  OUTLINE_COLOR,
  OUTLINE_THICKNESS,
  MODEL,
  VIEW,
  PROJECTION,
  TEXTURE0,
  SUBROUTINE,
  COUNT, // must be last
};

/// Enumeration of supported Shader Subroutines
enum class GLSub {
  TEXTURE,
  FONT,
  COLOR,
};

/// GLShader represents an OpenGL shader program
class GLShader final {

  explicit GLShader(std::string name);

 public:
  ~GLShader();

  // Movable but not Copyable
  GLShader(GLShader&& o) = default;
  GLShader(const GLShader&) = delete;
  GLShader& operator=(GLShader&&) = default;
  GLShader& operator=(const GLShader&) = delete;

  /// Get shader program name
  [[nodiscard]] std::string_view name() const { return name_; }

  /// Build a shader program from sources
  static auto build(std::string name, std::string_view vert_src, std::string_view frag_src) -> std::optional<GLShader>;

  /// Bind shader program
  void bind();

  /// Unbind shader program
  void unbind();

  /// Get attribute location
  [[nodiscard]] GLint attr_loc(GLAttr attr) const { return attrs_[static_cast<size_t>(attr)]; }

  /// Get uniform location
  [[nodiscard]] GLint unif_loc(GLUnif unif) const { return unifs_[static_cast<size_t>(unif)]; }

  /// Load attributes' location into local array
  void load_attr_loc(GLAttr attr, std::string_view attr_name);

  /// Load uniforms' location into local array
  void load_unif_loc(GLUnif unif, std::string_view unif_name);

 private:
  /// Compile a single shader from sources
  auto compile(GLenum shader_type, const char* shader_src) -> std::optional<GLuint>;

  /// Link shaders into program object
  bool link(GLuint vert, GLuint frag);

  /// Stringify opengl shader type.
  static auto shader_type_str(GLenum shader_type) -> std::string_view;

 private:
  /// Program name
  std::string name_;
  /// Program ID
  UniqueNum<unsigned int> id_;
  /// Attributes' location
  std::array<GLint, static_cast<size_t>(GLAttr::COUNT)> attrs_ = { -1 };
  /// Uniforms' location
  std::array<GLint, static_cast<size_t>(GLUnif::COUNT)> unifs_ = { -1 };
};

