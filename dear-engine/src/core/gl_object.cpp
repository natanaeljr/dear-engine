#include "gl_object.hpp"

#include <glbinding/gl33core/gl.h>
using namespace gl;

#include "gl_shader.hpp"

/// Upload new Colored Indexed-Vertex object to GPU memory
GLObject create_colored_globject(const GLShader& shader, gsl::span<const ColorVertex> vertices, gsl::span<const GLushort> indices, GLenum usage)
{
  GLuint vbo, ebo, vao;
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size_bytes(), vertices.data(), usage);
  glEnableVertexAttribArray(shader.attr_loc(GLAttr::POSITION));
  glVertexAttribPointer(shader.attr_loc(GLAttr::POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*) offsetof(ColorVertex, pos));
  glEnableVertexAttribArray(shader.attr_loc(GLAttr::COLOR));
  glVertexAttribPointer(shader.attr_loc(GLAttr::COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*) offsetof(ColorVertex, color));
  glDisableVertexAttribArray(shader.attr_loc(GLAttr::TEXCOORD));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), usage);
  return { vbo, ebo, vao, indices.size(), vertices.size() };
}

/// Upload new Textured Indexed-Vertex object to GPU memory
GLObject create_textured_globject(const GLShader& shader, gsl::span<const TextureVertex> vertices, gsl::span<const GLushort> indices, GLenum usage)
{
  GLuint vbo, ebo, vao;
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size_bytes(), vertices.data(), usage);
  glEnableVertexAttribArray(shader.attr_loc(GLAttr::POSITION));
  glVertexAttribPointer(shader.attr_loc(GLAttr::POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*) offsetof(TextureVertex, pos));
  glEnableVertexAttribArray(shader.attr_loc(GLAttr::TEXCOORD));
  glVertexAttribPointer(shader.attr_loc(GLAttr::TEXCOORD), 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*) offsetof(TextureVertex, texcoord));
  glDisableVertexAttribArray(shader.attr_loc(GLAttr::COLOR));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), usage);
  return { vbo, ebo, vao, indices.size(), vertices.size() };
}

/// Upload new colored Quad object to GPU memory
GLObject create_colored_quad_globject(const GLShader& shader, GLenum usage)
{
  return create_colored_globject(shader, kColorQuadVertices, kQuadIndices, usage);
}

/// Upload new textured Quad object to GPU memory
GLObject create_textured_quad_globject(const GLShader& shader, GLenum usage)
{
  return create_textured_globject(shader, kTextureQuadVertices, kQuadIndices, usage);
}

