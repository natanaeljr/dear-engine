#include "./gl_font.hpp"

#include <string>
#include <optional>

#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>

#include "./log.hpp"
#include "./file.hpp"
#include "./gl_texture.hpp"

using namespace std::string_literals;

/// Read font file and upload generated bitmap texture to GPU memory
auto load_font(const std::string& fontname) -> std::optional<GLFont>
{
  DEBUG("Loading Font {}", fontname);
  const std::string filepath = INMATH_ASSETS_PATH + "/fonts/"s + fontname;
  auto font = read_file_to_string(filepath);
  if (!font) { ERROR("Failed to load font '{}'", fontname); return std::nullopt; }
  constexpr int kStride = 0;
  constexpr int kPadding = 2;
  constexpr unsigned int kOversampling = 2;
  constexpr float kPixelHeight = 22.0;
  constexpr int kCharBeg = 32;
  constexpr int kCharEnd = 128;
  constexpr int kCharCount = kCharEnd - kCharBeg;
  const int kBitmapPixelSize = std::sqrt(kPixelHeight * kPixelHeight * (2.f/3.f) * kCharCount) * kOversampling;
  uint8_t bitmap[kBitmapPixelSize * kBitmapPixelSize];
  stbtt_pack_context pack_ctx;
  stbtt_packedchar packed_chars[kCharCount];
  stbtt_PackBegin(&pack_ctx, bitmap, kBitmapPixelSize, kBitmapPixelSize, kStride, kPadding, nullptr);
  stbtt_PackSetOversampling(&pack_ctx, kOversampling, kOversampling);
  int ret = stbtt_PackFontRange(&pack_ctx, (const uint8_t*)font->data(), 0, STBTT_POINT_SIZE(kPixelHeight), kCharBeg, kCharCount, packed_chars);
  if (ret <= 0) WARN("Font '{}': Some characters may not have fit in the font bitmap!", fontname);
  stbtt_PackEnd(&pack_ctx);
  std::vector<stbtt_packedchar> chars;
  chars.reserve(kCharCount);
  std::copy_n(packed_chars, kCharCount, std::back_inserter(chars));
  GLTexture texture = load_font_texture(bitmap, kBitmapPixelSize, kBitmapPixelSize);
  return GLFont{ 
    .texture = std::move(texture),
    .bitmap_px_width = kBitmapPixelSize,
    .bitmap_px_height = kBitmapPixelSize,
    .char_beg = kCharBeg,
    .char_count = kCharCount,
    .chars = std::move(chars),
    .pixel_height = kPixelHeight,
  };
}
