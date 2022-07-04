#pragma once

#include <glm/vec2.hpp>

#include "window.hpp"
#include "viewport.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Cursor

struct Cursor {
  glm::vec2 pos;

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
  glm::vec2 normalized(const Window& window, const Viewport& viewport) {
    float normal_max_width = 2.f * window.aspect_ratio();
    float normal_max_height = 2.f;
    float x = (((pos.x - viewport.offset.x) * normal_max_width) / viewport.size.x) - window.aspect_ratio();
    float y = (((pos.y - viewport.offset.y) * -normal_max_height) / viewport.size.y) + 1.f;
    return {x, y};
  }
};

