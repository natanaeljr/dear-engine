#pragma once

#include <string>
#include <optional>
#include <unordered_map>

#include "core/gl_texture.hpp"
#include "core/res_manager.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Textures

/// Holds the textures used by the game
class Textures: public ResManager<std::string, GLTextureRef> {
  using Base = ResManager<std::string, GLTextureRef>;

 public:
  Textures() = default;
  virtual ~Textures() = default;

  /// Load a Texture into cache
  template<typename ...Args>
  auto load(const std::string& texpath, Args&&... args) -> std::optional<GLTextureRef> {
    auto tex = load_rgba_texture(texpath, std::forward<Args>(args)...);
    if (!tex) return std::nullopt;
    return Base::load(texpath, std::make_shared<GLTexture>(std::move(*tex)));
  }
};

