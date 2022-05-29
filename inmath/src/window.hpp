#pragma once

#include <glm/vec2.hpp>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Window

struct Window {
  struct GLFWwindow* glfw;
  glm::uvec2 size;

  /// Get window aspect ratio
  float aspect_ratio() const { return (float)size.x / size.y; }
  float aspect_ratio_inverse() const { return (float)size.y / size.x; }
};
