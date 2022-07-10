#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <yaml-cpp/node/node.h>

#include "core/gl_font.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Components

/// Tag component
struct Tag {
  char label[32]{"?"};
};

/// Transform component
struct Transform {
  glm::vec2 position {0.0f};
  glm::vec2 scale    {0.5f};
  float rotation     {0.0f};

  glm::mat4 matrix() {
    glm::mat4 translation_mat = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
    glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(scale, 1.0f));
    return translation_mat * rotation_mat * scale_mat;
  }
};

namespace YAML {
template<>
struct convert<Transform> {
  static Node encode(const Transform& transform) {
    Node node;
    node["position"].push_back(transform.position.x);
    node["position"].push_back(transform.position.y);
    node["scale"].push_back(transform.scale.x);
    node["scale"].push_back(transform.scale.y);
    node["rotation"].push_back(transform.rotation);
    return node;
  }

  static bool decode(const Node& node, Transform& transform) {
    auto position = node["position"];
    auto scale = node["scale"];
    auto rotation = node["rotation"];
    if (!position || !scale || !rotation) return false;
    transform.position.x = position[0].as<float>();
    transform.position.y = position[1].as<float>();
    transform.scale.x = scale[0].as<float>();
    transform.scale.y = scale[1].as<float>();
    transform.rotation = rotation.as<float>();
    return true;
  }
};
}

/// Motion component
struct Motion {
  glm::vec2 velocity     {0.0f};
  glm::vec2 acceleration {0.0f};
};

/// Text Format component
struct TextFormat {
  GLFontRef font;
  glm::vec4 color         {1.0f};
  glm::vec4 outline_color {glm::vec3(0.0f), 1.0f};
  float outline_thickness {1.0f};
};

/// Custom Update Function component
struct UpdateFn {
  std::function<void(struct GameObject& obj, float dt, float time)> fn  { [](auto&&...){} };
};

/// Timed Action component
struct TimedAction {
  float tick_dt;  // deltatime between last round and now
  float duration; // time to go off, in seconds
  std::function<void(struct Game& game, float dt, float time)> action  { [](auto&&...){} };

  /// Update timer and invoke action on expire
  void update(struct Game& game, float dt, float time) {
    tick_dt += dt;
    if (tick_dt >= duration) {
      tick_dt -= duration;
      action(game, dt, time);
    }
  }
};

/// Off-Screen Destroy component
struct OffScreenDestroy { };

/// Screen Bound component
struct ScreenBound { };

/// Delay Erasing component
struct DelayErasing {
  bool sound = true;
};

/// Health component
struct Health {
  int value;
};
