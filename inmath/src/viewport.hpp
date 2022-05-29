#pragma once

#include <glm/vec2.hpp>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Viewport

///         viewport.offset.x
///        |-------|
///        +------------------------+ - viewport.offset.y
///        | (0,0) +-------+ (W, 0) | -
///        |       | view  |        | |
/// window |       | port  |        | | viewport.size.y
///  space |       | space |        | |
///        | (0,H) +-------+ (W, H) | -
///        +------------------------+
///                |-------| viewport.size.x

struct Viewport {
  glm::uvec2 offset;
  glm::uvec2 size;

  /// Get viewport aspect ratio
  float aspect_ratio() const { return (float)size.x / size.y; }
  float aspect_ratio_inverse() const { return (float)size.y / size.x; }
};

