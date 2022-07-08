#pragma once

#include <memory>

#include <AL/al.h>
#include <AL/alc.h>

#include "unique_num.hpp"
#include "al_buffer.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Audio Source

/// Represents the origin of an audio sound in the world position, that's the play source of an audio buffer
struct ALSource {
  UniqueNum<ALuint> id;
  ALBufferRef buf;

  ~ALSource() {
    if (id) alDeleteSources(1, &id.inner);
  }

  void bind_buffer(ALBufferRef _buf) {
    alSourcei(id, AL_BUFFER, _buf->id);
    this->buf = std::move(_buf);
  }

  void play() { alSourcePlay(id); }

  // Movable but not Copyable
  ALSource(ALSource&&) = default;
  ALSource(const ALSource&) = delete;
  ALSource& operator=(ALSource&&) = default;
  ALSource& operator=(const ALSource&) = delete;
};

/// ALSource reference type alias
using ALSourceRef = std::shared_ptr<ALSource>;

/// Create a new Audio Source object
ALSource create_audio_source(float gain);

