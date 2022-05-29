#include <string>
#include <optional>
#include <unordered_map>

#include "./al_audio.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Audios

/// Holds the audio buffers used by the game
struct Audios {
  std::unordered_map<std::string, ALBufferRef> map;

 public:
  /// Get an Audio buffer from cache, or load it if not cached yet
  template<typename ...Args>
  auto get_or_load(const std::string& audiopath, Args&&... args) -> std::optional<ALBufferRef> {
    auto it = map.find(audiopath);
    if (it != map.end()) {
      return it->second;
    } else {
      auto audio = load_wav_audio(audiopath, std::forward<Args>(args)...);
      if (!audio) return std::nullopt;
      auto audioptr = std::make_shared<ALBuffer>(std::move(*audio));
      map.emplace(audiopath, audioptr);
      return audioptr;
    }
  }
};

