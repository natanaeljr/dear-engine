#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Collision

/// Axis-aligned Bounding Box component
/// in respect to the object's local space
/// (no rotation support)
///     +---+ max
///     | x |
/// min +---+    x = center = origin = transform.position
struct Aabb {
  glm::vec2 min {-1.0f};
  glm::vec2 max {+1.0f};

  Aabb transform(const glm::mat4& matrix) const {
    glm::vec2 a = matrix * glm::vec4(min, 0.0f, 1.0f);
    glm::vec2 b = matrix * glm::vec4(max, 0.0f, 1.0f);
    return Aabb{glm::min(a, b), glm::max(a, b)};
  }
};

/// Check for collision between two AABBs
bool collision(const Aabb& a, const Aabb& b);

