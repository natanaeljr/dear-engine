#pragma once

#include <string>
#include <optional>
#include <unordered_map>

#include "./gl_texture.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Textures

/// Holds the textures used by the game
class Textures {
  std::unordered_map<std::string, GLTextureRef> map;

 public:
  /// Get a Texture ID from cache, or load it if not cached yet
  template<typename ...Args>
  auto get_or_load(const std::string& texpath, Args&&... args) -> std::optional<GLTextureRef> {
    auto it = map.find(texpath);
    if (it != map.end()) {
      return it->second;
    } else {
      auto tex = load_rgba_texture(texpath, std::forward<Args>(args)...);
      if (!tex) return std::nullopt;
      auto texptr = std::make_shared<GLTexture>(std::move(*tex));
      map.emplace(texpath, texptr);
      return texptr;
    }
  }
};

