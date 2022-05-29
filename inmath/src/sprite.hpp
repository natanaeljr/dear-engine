#pragma once

#include <glbinding/gl33core/types.h>
using namespace gl;

#include "./gl_object.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Sprites

/// Generate quad vertices for a spritesheet texture with frames laid out linearly.
/// count=3:        .texcoord (U,V)
/// (0,1) +-----+-----+-----+ (1,1)
///       |     |     |     |
///       |  1  |  2  |  3  |
///       |     |     |     |
/// (0,0) +-----+-----+-----+ (1,0)
auto gen_sprite_quads(size_t count) -> std::tuple<std::vector<TextureVertex>, std::vector<GLushort>>;

/// Information required to render one frame of a Sprite Animation
struct SpriteFrame {
  float duration;    // duration in seconds, negative is infinite
  size_t ebo_offset; // offset to the first index of this frame in the EBO
  size_t ebo_count;  // number of elements to render since first index
};

/// Control data required for a single Sprite Animation object
struct SpriteAnimation {
  float last_transit_dt; // deltatime between last transition and now
  size_t curr_frame_idx; // current frame index
  std::vector<SpriteFrame> frames;
  size_t curr_cycle_count; // number of cycles executed
  size_t max_cycles; // max number of cycles to execute before ending sprite animation, zero for endless

  /// Transition frames
  void update_frame(float dt) {
    last_transit_dt += dt;
    SpriteFrame& curr_frame = frames[curr_frame_idx];
    if (last_transit_dt >= curr_frame.duration) {
      last_transit_dt -= curr_frame.duration;
      if (++curr_frame_idx == frames.size()) {
        curr_frame_idx = 0;
        curr_cycle_count++;
      }
    }
  }

  /// Get current sprite frame
  SpriteFrame& curr_frame() { return frames[curr_frame_idx]; }

  /// Check if animation already ran to maximum number of cycles
  bool expired() { return max_cycles > 0 && curr_cycle_count >= max_cycles; }
};

