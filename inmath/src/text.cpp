#include "./text.hpp"

#include <string_view>

#include <glbinding/gl33core/types.h>
using namespace gl;

#include "./gl_font.hpp"
#include "./gl_object.hpp"

/// Generate quad vertices for a text with the given font.
auto gen_text_quads(const GLFont& font, std::string_view text) -> std::tuple<std::vector<TextureVertex>, std::vector<GLushort>, float>
{
  std::vector<TextureVertex> vertices;
  vertices.reserve(4 * text.size());
  std::vector<GLushort> indices;
  indices.reserve(6 * text.size());
  float x = 0, y = 0;
  float width = 0;
  size_t i = 0;
  for (char ch : text) {
    if (ch < font.char_beg || ch > (font.char_beg + font.char_count))
      continue;
    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(font.chars.data(), font.bitmap_px_width, font.bitmap_px_height, (int)ch - font.char_beg, &x, &y, &q, 1);
    vertices.emplace_back(TextureVertex{ .pos = { q.x0, q.y0 }, .texcoord = { q.s0, q.t0 } });
    vertices.emplace_back(TextureVertex{ .pos = { q.x1, q.y0 }, .texcoord = { q.s1, q.t0 } });
    vertices.emplace_back(TextureVertex{ .pos = { q.x1, q.y1 }, .texcoord = { q.s1, q.t1 } });
    vertices.emplace_back(TextureVertex{ .pos = { q.x0, q.y1 }, .texcoord = { q.s0, q.t1 } });
    for (auto v : kQuadIndices)
      indices.emplace_back(4*i+v);
    width = q.x1;
    i++;
  }
  return std::tuple{vertices, indices, width};
}

/// Update GLObject data with the new generated quads for the given Text
void update_text_globject(const GLShader& shader, GLObject& glo, const GLFont& font, std::string_view text, GLenum usage)
{
  auto [vertices, indices, _] = gen_text_quads(font, text);
  if (vertices.size() == glo.num_vertices && indices.size() == glo.num_indices) {
    glBindVertexArray(glo.vao);
    glBindBuffer(GL_ARRAY_BUFFER, glo.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(TextureVertex), vertices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glo.ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLushort), indices.data());
  } else {
    glo = create_text_globject(shader, vertices, indices, usage);
  }
}

/// Upload new Text Indexed-Vertex object to GPU memory
auto create_text_globject(const GLShader& shader, gsl::span<const TextureVertex> vertices, gsl::span<const GLushort> indices, GLenum usage) -> GLObject
{
  return create_textured_globject(shader, vertices, indices, usage);
}
