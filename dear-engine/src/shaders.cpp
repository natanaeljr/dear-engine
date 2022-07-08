#include "./shaders.hpp"

#include "core/log.hpp"

/// Loads all shaders used by the game
Shaders load_shaders()
{
  return {
    .generic_shader = load_generic_shader(),
  };
}

/// Load Generic Shader
/// (supports rendering: Colored objects, Textured objects and BitmapFont text)
auto load_generic_shader() -> GLShader
{
  static constexpr std::string_view kShaderVert = R"(
#version 330 core
in vec2 aPosition;
in vec2 aTexCoord;
in vec4 aColor;
out vec4 fColor;
out vec2 fTexCoord;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
void main()
{
  gl_Position = uProjection * uView * uModel * vec4(aPosition, 0.0f, 1.0f);
  fTexCoord = aTexCoord;
  fColor = aColor;
}
)";

  static constexpr std::string_view kShaderFrag = R"(
#version 330 core
in vec2 fTexCoord;
in vec4 fColor;
out vec4 outColor;
uniform sampler2D uTexture0;
uniform vec4 uColor;
uniform vec4 uOutlineColor;
uniform float uOutlineThickness;
uniform int uSubRoutine;
vec4 font_color();
void main()
{
  if (uSubRoutine == 0) {
    outColor = texture(uTexture0, fTexCoord);
  } else if (uSubRoutine == 1) {
    outColor = font_color();
  } else if (uSubRoutine == 2) {
    outColor = fColor;
  } else {
    outColor = uColor;
  }
}
vec4 font_color()
{
  vec2 Offset = 1.0 / textureSize(uTexture0, 0) * uOutlineThickness;
  vec4 n = texture2D(uTexture0, vec2(fTexCoord.x, fTexCoord.y - Offset.y));
  vec4 e = texture2D(uTexture0, vec2(fTexCoord.x + Offset.x, fTexCoord.y));
  vec4 s = texture2D(uTexture0, vec2(fTexCoord.x, fTexCoord.y + Offset.y));
  vec4 w = texture2D(uTexture0, vec2(fTexCoord.x - Offset.x, fTexCoord.y));
  vec4 TexColor = vec4(vec3(1.0), texture2D(uTexture0, fTexCoord).r);
  float GrowedAlpha = TexColor.a;
  GrowedAlpha = mix(GrowedAlpha, 1.0, s.r);
  GrowedAlpha = mix(GrowedAlpha, 1.0, w.r);
  GrowedAlpha = mix(GrowedAlpha, 1.0, n.r);
  GrowedAlpha = mix(GrowedAlpha, 1.0, e.r);
  vec4 OutlineColorWithNewAlpha = vec4(uOutlineColor.rgb, uOutlineColor.a * GrowedAlpha);
  vec4 CharColor = TexColor * uColor;
  return mix(OutlineColorWithNewAlpha, CharColor, CharColor.a);
}
)";

  DEBUG("Loading Generic Shader");
  auto shader = GLShader::build("GenericShader", kShaderVert, kShaderFrag);
  ASSERT(shader);
  shader->bind();
  shader->load_attr_loc(GLAttr::POSITION, "aPosition");
  shader->load_attr_loc(GLAttr::TEXCOORD, "aTexCoord");
  shader->load_attr_loc(GLAttr::COLOR, "aColor");
  shader->load_unif_loc(GLUnif::MODEL, "uModel");
  shader->load_unif_loc(GLUnif::VIEW, "uView");
  shader->load_unif_loc(GLUnif::PROJECTION, "uProjection");
  shader->load_unif_loc(GLUnif::TEXTURE0, "uTexture0");
  shader->load_unif_loc(GLUnif::COLOR, "uColor");
  shader->load_unif_loc(GLUnif::OUTLINE_COLOR, "uOutlineColor");
  shader->load_unif_loc(GLUnif::OUTLINE_THICKNESS, "uOutlineThickness");
  shader->load_unif_loc(GLUnif::SUBROUTINE, "uSubRoutine");

  return std::move(*shader);
}

