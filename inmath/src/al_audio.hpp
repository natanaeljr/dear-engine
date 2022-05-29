#pragma once

#include <memory>
#include <string>
#include <optional>

#include <glbinding/gl33core/gl.h>
using namespace gl;

#include <AL/al.h>
#include <AL/alc.h>

#include "./unique_num.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Audio

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

