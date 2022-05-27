#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Camera

struct Camera {
  glm::mat4 projection;
  glm::mat4 view;

  /// Create Orthographic Camera
  static Camera create(float aspect_ratio) {
    const float zoom_level = 1.0f;
    const float rotation = 0.0f;
    return {
      .projection = glm::ortho(-aspect_ratio * zoom_level, +aspect_ratio * zoom_level, -zoom_level, +zoom_level, +1.0f, -1.0f),
      .view = glm::inverse(glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0, 0, 1))),
    };
  }
};

/// Upload camera matrix to shader
void set_camera(const class GLShader& shader, const Camera& camera);

