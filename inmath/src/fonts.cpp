#include "./fonts.hpp"

#include "./log.hpp"

/// Loads all fonts used by the game
Fonts load_fonts()
{
  return {
    .menlo = std::make_shared<GLFont>(*ASSERT_GET(load_font("Menlo-Regular.ttf"))),
    .jetbrains = std::make_shared<GLFont>(*ASSERT_GET(load_font("JetBrainsMono-Regular.ttf"))),
    .google_sans = std::make_shared<GLFont>(*ASSERT_GET(load_font("GoogleSans-Regular.ttf"))),
    .kanit = std::make_shared<GLFont>(*ASSERT_GET(load_font("Kanit/Kanit-Bold.ttf"))),
    .russo_one = std::make_shared<GLFont>(*ASSERT_GET(load_font("Russo_One/RussoOne-Regular.ttf"))),
  };
}
