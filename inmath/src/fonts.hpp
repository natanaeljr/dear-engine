#pragma once

#include "core/gl_font.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Fonts

/// Holds the fonts used by the game
struct Fonts {
  GLFontRef menlo;
  GLFontRef jetbrains;
  GLFontRef google_sans;
  GLFontRef kanit;
  GLFontRef russo_one;
};

/// Loads all fonts used by the game
Fonts load_fonts();

