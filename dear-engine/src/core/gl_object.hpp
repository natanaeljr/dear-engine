#pragma once

#include <memory>

#include <glbinding/gl33core/gl.h>
using namespace gl;

#include <gsl/span>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "gl_shader.hpp"
#include "unique_num.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GLObjects

/// Vertex representation for a Colored Object
struct ColorVertex {
  glm::vec2 pos;
  glm::vec4 color;
};

/// Vertex representation for a Textured Object
struct TextureVertex {
  glm::vec2 pos;
  glm::vec2 texcoord;
};

/// Represents an object loaded to GPU memory that's renderable using indices
struct GLObject {
  UniqueNum<GLuint> vbo;
  UniqueNum<GLuint> ebo;
  UniqueNum<GLuint> vao;
  size_t num_indices;
  size_t num_vertices;

  ~GLObject() {
    if (vbo) glDeleteBuffers(1, &vbo.inner);
    if (ebo) glDeleteBuffers(1, &ebo.inner);
    if (vao) glDeleteVertexArrays(1, &vao.inner);
  }

  // Movable but not Copyable
  GLObject(GLObject&&) = default;
  GLObject(const GLObject&) = delete;
  GLObject& operator=(GLObject&&) = default;
  GLObject& operator=(const GLObject&) = delete;
};

/// GLObject reference type alias
using GLObjectRef = std::shared_ptr<GLObject>;

/// Upload new Colored Indexed-Vertex object to GPU memory
GLObject create_colored_globject(const GLShader& shader, gsl::span<const ColorVertex> vertices, gsl::span<const GLushort> indices, GLenum usage = GL_STATIC_DRAW);

/// Upload new Textured Indexed-Vertex object to GPU memory
GLObject create_textured_globject(const GLShader& shader, gsl::span<const TextureVertex> vertices, gsl::span<const GLushort> indices, GLenum usage = GL_STATIC_DRAW);

// Quad Vertices:
// (-1,+1)       (+1,+1)
//  Y ^ - - - - - - o
//    |  D       A  |
//    |   +-----+   |
//    |   | \   |   |
//    |   |  0  |   |
//    |   |   \ |   |
//    |   +-----+   |
//    |  C       B  |
//    o - - - - - - > X
// (-1,-1)       (+1,-1)
// positive Z goes through screen towards you

inline constexpr ColorVertex kColorQuadVertices[] = {
  { .pos = { +1.0f, +1.0f }, .color = { 0.0f, 0.0f, 1.0f, 1.0f } },
  { .pos = { +1.0f, -1.0f }, .color = { 0.0f, 1.0f, 0.0f, 1.0f } },
  { .pos = { -1.0f, -1.0f }, .color = { 1.0f, 0.0f, 0.0f, 1.0f } },
  { .pos = { -1.0f, +1.0f }, .color = { 1.0f, 0.0f, 1.0f, 1.0f } },
};

inline constexpr TextureVertex kTextureQuadVertices[] = {
  { .pos = { +1.0f, +1.0f }, .texcoord = { 1.0f, 1.0f } },
  { .pos = { +1.0f, -1.0f }, .texcoord = { 1.0f, 0.0f } },
  { .pos = { -1.0f, -1.0f }, .texcoord = { 0.0f, 0.0f } },
  { .pos = { -1.0f, +1.0f }, .texcoord = { 0.0f, 1.0f } },
};

inline constexpr GLushort kQuadIndices[] = {
  0, 1, 3,
  1, 2, 3,
};

/// Upload new colored Quad object to GPU memory
GLObject create_colored_quad_globject(const GLShader& shader, GLenum usage = GL_STATIC_DRAW);

/// Upload new textured Quad object to GPU memory
GLObject create_textured_quad_globject(const GLShader& shader, GLenum usage = GL_STATIC_DRAW);

