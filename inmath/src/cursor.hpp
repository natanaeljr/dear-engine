#pragma once

#include <glm/vec2.hpp>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Cursor

struct Cursor {
  glm::vec2 pos;
};

/// Convert cursor position from window space to normalized space (-1,+1)
///
///  viewport_offset.x
/// |-------|
/// +------------------------+ - viewport_offset.y
/// | (0,0) +-------+ (W, 0) | -
/// |       | view  |        |
/// |       | port  |        |
/// |       | space |        |
/// | (0,H) +-------+ (W, H) |
/// +------------------------+
///       window space
glm::vec2 normalized_cursor_pos(glm::vec2 cursor, glm::uvec2 viewport_size, glm::uvec2 viewport_offset);

