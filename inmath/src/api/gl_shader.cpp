#include "./gl_shader.hpp"

#include <array>
#include <string>
#include <optional>

#include <glbinding/gl33core/gl.h>
using namespace gl;

#include "./log.hpp"
#include "./unique_num.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Shader

GLShader::GLShader(std::string name)
    : name_(std::move(name)), id_(glCreateProgram())
{
  TRACE("New GLShader program '{}'[{}]", name_, id_);
}

GLShader::~GLShader()
{
  if (id_) {
    glDeleteProgram(id_);
    TRACE("Delete GLShader program '{}'[{}]", name_, id_);
  }
}

void GLShader::bind() { glUseProgram(id_); }

void GLShader::unbind() { glUseProgram(0); }

void GLShader::load_attr_loc(GLAttr attr, std::string_view attr_name)
{
  const GLint loc = glGetAttribLocation(id_, attr_name.data());
  if (loc == -1) ABORT_MSG("Failed to get location for attribute '{}' GLShader '{}'[{}]", attr_name, name_, id_);
  TRACE("Loaded attritube '{}' location {} GLShader '{}'[{}]", attr_name, loc, name_, id_);
  attrs_[static_cast<size_t>(attr)] = loc;
}

void GLShader::load_unif_loc(GLUnif unif, std::string_view unif_name)
{
  const GLint loc = glGetUniformLocation(id_, unif_name.data());
  if (loc == -1) ABORT_MSG("Failed to get location for uniform '{}' GLShader '{}'[{}]", unif_name, name_, id_);
  TRACE("Loaded uniform '{}' location {} GLShader '{}'[{}]", unif_name, loc, name_, id_);
  unifs_[static_cast<size_t>(unif)] = loc;
}

auto GLShader::build(std::string name, std::string_view vert_src, std::string_view frag_src) -> std::optional<GLShader>
{
  auto shader = GLShader(std::move(name));
  auto vertex = shader.compile(GL_VERTEX_SHADER, vert_src.data());
  auto fragment = shader.compile(GL_FRAGMENT_SHADER, frag_src.data());
  if (!vertex || !fragment) {
    ERROR("Failed to Compile Shaders for program '{}'[{}]", shader.name_, shader.id_);
    if (vertex) glDeleteShader(*vertex);
    if (fragment) glDeleteShader(*fragment);
    return std::nullopt;
  }
  if (!shader.link(*vertex, *fragment)) {
    ERROR("Failed to Link GLShader program '{}'[{}]", shader.name_, shader.id_);
    glDeleteShader(*vertex);
    glDeleteShader(*fragment);
    return std::nullopt;
  }
  glDeleteShader(*vertex);
  glDeleteShader(*fragment);
  TRACE("Compiled&Linked shader program '{}'[{}]", shader.name_, shader.id_);
  return shader;
}

auto GLShader::compile(GLenum shader_type, const char *shader_src) -> std::optional<GLuint>
{
  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &shader_src, nullptr);
  glCompileShader(shader);
  GLint info_len = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
  if (info_len) {
    auto info = std::make_unique<char[]>(info_len);
    glGetShaderInfoLog(shader, info_len, nullptr, info.get());
    DEBUG("GLShader '{}'[{}] Compilation Output {}:\n{}", name_, id_, shader_type_str(shader_type), info.get());
  }
  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    ERROR("Failed to Compile {} for GLShader '{}'[{}]", shader_type_str(shader_type), name_, id_);
    glDeleteShader(shader);
    return std::nullopt;
  }
  return shader;
}

bool GLShader::link(GLuint vert, GLuint frag) {
  glAttachShader(id_, vert);
  glAttachShader(id_, frag);
  glLinkProgram(id_);
  GLint info_len = 0;
  glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &info_len);
  if (info_len) {
    auto info = std::make_unique<char[]>(info_len);
    glGetProgramInfoLog(id_, info_len, nullptr, info.get());
    DEBUG("GLShader '{}'[{}] Program Link Output:\n{}", name_, id_, info.get());
  }
  GLint link_status = 0;
  glGetProgramiv(id_, GL_LINK_STATUS, &link_status);
  if (!link_status)
    ERROR("Failed to Link GLShader Program '{}'[{}]", name_, id_);
  glDetachShader(id_, vert);
  glDetachShader(id_, frag);
  return link_status;
}

auto GLShader::shader_type_str(GLenum shader_type) -> std::string_view {
  switch (shader_type) {
  case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
  case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
  default:
    ABORT_MSG("Invalid shader type {}", (int)shader_type);
    return "<invalid>";
  }
}

