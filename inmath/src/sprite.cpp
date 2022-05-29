#include "./sprite.hpp"

#include "./gl_object.hpp"

/// Generate quad vertices for a spritesheet texture with frames laid out linearly.
/// count=3:        .texcoord (U,V)
/// (0,1) +-----+-----+-----+ (1,1)
///       |     |     |     |
///       |  1  |  2  |  3  |
///       |     |     |     |
/// (0,0) +-----+-----+-----+ (1,0)
auto gen_sprite_quads(size_t count) -> std::tuple<std::vector<TextureVertex>, std::vector<GLushort>>
{
  float width = 1.0f / count;
  std::vector<TextureVertex> vertices;
  std::vector<GLushort> indices;
  vertices.reserve(4 * count);
  indices.reserve(6 * count);
  for (size_t i = 0; i < count; i++) {
    vertices.emplace_back(TextureVertex{ .pos = { +1.0f, +1.0f }, .texcoord = { (i+1)*width, 1.0f } });
    vertices.emplace_back(TextureVertex{ .pos = { +1.0f, -1.0f }, .texcoord = { (i+1)*width, 0.0f } });
    vertices.emplace_back(TextureVertex{ .pos = { -1.0f, -1.0f }, .texcoord = { (i+0)*width, 0.0f } });
    vertices.emplace_back(TextureVertex{ .pos = { -1.0f, +1.0f }, .texcoord = { (i+0)*width, 1.0f } });
    for (auto v : kQuadIndices)
      indices.emplace_back(4*i+v);
  }
  return {vertices, indices};
}
