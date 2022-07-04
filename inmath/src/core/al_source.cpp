#include "al_source.hpp"

#include <AL/al.h>
#include <AL/alc.h>

/// Create a new Audio Source object
ALSource create_audio_source(float gain)
{
  ALuint aso;
  alGenSources(1, &aso);
  alSourcef(aso, AL_PITCH, 1.0f);
  alSourcef(aso, AL_GAIN, gain);
  alSource3f(aso, AL_POSITION, 0.0f, 0.0f, 0.0f);
  alSource3f(aso, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
  alSourcei(aso, AL_LOOPING, AL_FALSE);
  return ALSource{ aso };
}

