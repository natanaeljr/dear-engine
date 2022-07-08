#include <string>
#include <optional>
#include <unordered_map>

#include "core/al_buffer.hpp"
#include "core/al_source.hpp"
#include "core/res_manager.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Audios

/// Holds the audio buffers used by the game
class Audios : public ResManager<std::string, ALBufferRef> {
  using Base = ResManager<std::string, ALBufferRef>;

 public:
  Audios() = default;
  virtual ~Audios() = default;

  /// Load an Audio buffer into cache
  template <typename... Args>
  auto load(const std::string &audiopath, Args &&...args) -> std::optional<ALBufferRef> {
    auto audio = load_wav_audio(audiopath, std::forward<Args>(args)...);
    if (!audio) return std::nullopt;
    return Base::load(audiopath, std::make_shared<ALBuffer>(std::move(*audio)));
  }
};

