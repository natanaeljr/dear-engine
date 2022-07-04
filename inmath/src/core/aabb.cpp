#include "aabb.hpp"

/// Check for collision between two AABBs
bool collision(const Aabb& a, const Aabb& b)
{
  return a.min.x < b.max.x &&
         a.max.x > b.min.x &&
         a.min.y < b.max.y &&
         a.max.y > b.min.y;
}

