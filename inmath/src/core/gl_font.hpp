#pragma once

#include <stb/stb_truetype.h>

#include "gl_texture.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Font

/// Holds the a font bitmap texture and info needed to render char quads
struct GLFont {
  GLTexture texture;
  int bitmap_px_width;
  int bitmap_px_height;
  int char_beg;
  int char_count;
  std::vector<stbtt_packedchar> chars;
  float pixel_height;
};

/// GLFont reference type alias
using GLFontRef = std::shared_ptr<GLFont>;

/// Read font file and upload generated bitmap texture to GPU memory
auto load_font(const std::string& fontname) -> std::optional<GLFont>;

