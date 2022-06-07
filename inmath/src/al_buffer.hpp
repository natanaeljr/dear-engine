#pragma once

#include <memory>
#include <string>
#include <optional>

#include <AL/al.h>
#include <AL/alc.h>

#include "./unique_num.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Audio Buffer

/// Represents an audio buffer loaded into OpenAL
struct ALBuffer {
  UniqueNum<ALuint> id;

  ~ALBuffer() {
    if (id) alDeleteBuffers(1, &id.inner);
  }

  // Movable but not Copyable
  ALBuffer(ALBuffer&&) = default;
  ALBuffer(const ALBuffer&) = delete;
  ALBuffer& operator=(ALBuffer&&) = default;
  ALBuffer& operator=(const ALBuffer&) = delete;
};

/// ALBuffer reference type alias
using ALBufferRef = std::shared_ptr<ALBuffer>;

/// Read WAV audio file and load it into an OpenAL buffer
auto load_wav_audio(const std::string& audiopath) -> std::optional<ALBuffer>;

