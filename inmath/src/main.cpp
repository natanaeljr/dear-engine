#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <variant>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <unistd.h>

#include <glbinding/gl33core/gl.h>
#include <glbinding/glbinding.h>
#include <glbinding-aux/debug.h>
using namespace gl;

#include <AL/al.h>
#include <AL/alc.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>
#include <stb/stb_image.h>
#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>
#include <drlibs/dr_wav.h>
#include <gsl/span>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Utils

#define TRACE SPDLOG_TRACE
#define DEBUG SPDLOG_DEBUG
#define INFO SPDLOG_INFO
#define WARN SPDLOG_WARN
#define ERROR SPDLOG_ERROR
#define CRITICAL SPDLOG_CRITICAL
#define ABORT_MSG(...) do { CRITICAL(__VA_ARGS__); std::abort(); } while (0)
#define ASSERT_MSG(expr, ...) do { if (expr) { } else { ABORT_MSG(__VA_ARGS__); } } while (0)
#define ASSERT(expr) ASSERT_MSG(expr, "Assertion failed: ({})", #expr)
#define ASSERT_RET(expr) [&]{ auto ret = (expr); ASSERT_MSG(ret, "Assertion failed: ({})", #expr); return ret; }()
#define DBG(expr) [&]{ auto ret = (expr); DEBUG("({}) = {{{}}}", #expr, ret); return ret; }()

using namespace std::string_literals;

/// Read file contents to a string
auto read_file_to_string(const std::string& filename) -> std::optional<std::string>
{
  std::string string;
  std::fstream fstream(filename, std::ios::in | std::ios::binary);
  if (!fstream) { ERROR("{} ({})", std::strerror(errno), filename); return std::nullopt; }
  fstream.seekg(0, std::ios::end);
  string.reserve(fstream.tellg());
  fstream.seekg(0, std::ios::beg);
  string.assign((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
  return string;
}

/// Utility type to make unique numbers (IDs) movable, when moved the value should be zero
template<typename T>
struct UniqueNum {
  T inner;
  UniqueNum() : inner(0) {}
  UniqueNum(T n) : inner(n) {}
  UniqueNum(UniqueNum&& o) : inner(o.inner) { o.inner = 0; }
  UniqueNum& operator=(UniqueNum&& o) { inner = o.inner; o.inner = 0; return *this; }
  UniqueNum(const UniqueNum&) = delete;
  UniqueNum& operator=(const UniqueNum&) = delete;
  operator T() const { return inner; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Colors

[[maybe_unused]] constexpr auto kWhite = glm::vec4(1.f);
[[maybe_unused]] constexpr auto kWhiteDimmed = glm::vec4(glm::vec3(0.87f), 1.f);
[[maybe_unused]] constexpr auto kBlack = glm::vec4(glm::vec3(0.f), 1.f);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Shaders

/// Enumeration of supported Shader Attributes
enum class GLAttr {
  POSITION,
  COLOR,
  MODEL,
  TEXCOORD,
  COUNT, // must be last
};

/// Enumeration of supported Shader Uniforms
enum class GLUnif {
  COLOR,
  OUTLINE_COLOR,
  OUTLINE_THICKNESS,
  MODEL,
  VIEW,
  PROJECTION,
  TEXTURE0,
  SUBROUTINE,
  COUNT, // must be last
};

/// Enumeration of supported Shader Subroutines
enum class GLSub {
  TEXTURE,
  FONT,
  COLOR,
};

/// GLShader represents an OpenGL shader program
class GLShader final {
  /// Program name
  std::string name_;
  /// Program ID
  UniqueNum<unsigned int> id_;
  /// Attributes' location
  std::array<GLint, static_cast<size_t>(GLAttr::COUNT)> attrs_ = { -1 };
  /// Uniforms' location
  std::array<GLint, static_cast<size_t>(GLUnif::COUNT)> unifs_ = { -1 };

  public:
  explicit GLShader(std::string name) : name_(std::move(name)), id_(glCreateProgram()) {
    TRACE("New GLShader program '{}'[{}]", name_, id_);
  }
  ~GLShader() {
    if (id_) {
      glDeleteProgram(id_);
      TRACE("Delete GLShader program '{}'[{}]", name_, id_);
    }
  }
  // Movable but not Copyable
  GLShader(GLShader&& o) = default;
  GLShader(const GLShader&) = delete;
  GLShader& operator=(GLShader&&) = default;
  GLShader& operator=(const GLShader&) = delete;

  public:
  /// Get shader program name
  [[nodiscard]] std::string_view name() const { return name_; }

  /// Bind shader program
  void bind() { glUseProgram(id_); }
  /// Unbind shader program
  void unbind() { glUseProgram(0); }

  /// Get attribute location
  [[nodiscard]] GLint attr_loc(GLAttr attr) const { return attrs_[static_cast<size_t>(attr)]; }
  /// Get uniform location
  [[nodiscard]] GLint unif_loc(GLUnif unif) const { return unifs_[static_cast<size_t>(unif)]; }

  /// Load attributes' location into local array
  void load_attr_loc(GLAttr attr, std::string_view attr_name)
  {
    const GLint loc = glGetAttribLocation(id_, attr_name.data());
    if (loc == -1)
      ABORT_MSG("Failed to get location for attribute '{}' GLShader '{}'[{}]", attr_name, name_, id_);
    TRACE("Loaded attritube '{}' location {} GLShader '{}'[{}]", attr_name, loc, name_, id_);
    attrs_[static_cast<size_t>(attr)] = loc;
  }

  /// Load uniforms' location into local array
  void load_unif_loc(GLUnif unif, std::string_view unif_name)
  {
    const GLint loc = glGetUniformLocation(id_, unif_name.data());
    if (loc == -1)
      ABORT_MSG("Failed to get location for uniform '{}' GLShader '{}'[{}]", unif_name, name_, id_);
    TRACE("Loaded uniform '{}' location {} GLShader '{}'[{}]", unif_name, loc, name_, id_);
    unifs_[static_cast<size_t>(unif)] = loc;
  }

  public:
  /// Build a shader program from sources
  static auto build(std::string name, std::string_view vert_src, std::string_view frag_src) -> std::optional<GLShader>
  {
    auto shader = GLShader(std::move(name));
    auto vertex = shader.compile(GL_VERTEX_SHADER, vert_src.data());
    auto fragment = shader.compile(GL_FRAGMENT_SHADER, frag_src.data());
    if (!vertex || !fragment) {
      ERROR("Failed to Compile Shaders for program '{}'[{}]", shader.name_, shader.id_);
      if (vertex) glDeleteShader(*vertex);
      if (fragment) glDeleteShader(*fragment);
      return std::nullopt;
    }
    if (!shader.link(*vertex, *fragment)) {
      ERROR("Failed to Link GLShader program '{}'[{}]", shader.name_, shader.id_);
      glDeleteShader(*vertex);
      glDeleteShader(*fragment);
      return std::nullopt;
    }
    glDeleteShader(*vertex);
    glDeleteShader(*fragment);
    TRACE("Compiled&Linked shader program '{}'[{}]", shader.name_, shader.id_);
    return shader;
  }

  private:
  /// Compile a single shader from sources
  auto compile(GLenum shader_type, const char* shader_src) -> std::optional<GLuint>
  {
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &shader_src, nullptr);
    glCompileShader(shader);
    GLint info_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len) {
      auto info = std::make_unique<char[]>(info_len);
      glGetShaderInfoLog(shader, info_len, nullptr, info.get());
      DEBUG("GLShader '{}'[{}] Compilation Output {}:\n{}", name_, id_, shader_type_str(shader_type), info.get());
    }
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
      ERROR("Failed to Compile {} for GLShader '{}'[{}]", shader_type_str(shader_type), name_, id_);
      glDeleteShader(shader);
      return std::nullopt;
    }
    return shader;
  }

  /// Link shaders into program object
  bool link(GLuint vert, GLuint frag)
  {
    glAttachShader(id_, vert);
    glAttachShader(id_, frag);
    glLinkProgram(id_);
    GLint info_len = 0;
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len) {
      auto info = std::make_unique<char[]>(info_len);
      glGetProgramInfoLog(id_, info_len, nullptr, info.get());
      DEBUG("GLShader '{}'[{}] Program Link Output:\n{}", name_, id_, info.get());
    }
    GLint link_status = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &link_status);
    if (!link_status)
      ERROR("Failed to Link GLShader Program '{}'[{}]", name_, id_);
    glDetachShader(id_, vert);
    glDetachShader(id_, frag);
    return link_status;
  }

  /// Stringify opengl shader type.
  static auto shader_type_str(GLenum shader_type) -> std::string_view
  {
    switch (shader_type) {
      case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
      case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
      default: ASSERT_MSG(0, "Invalid shader type {}", (int)shader_type); return "<invalid>";
    }
  }
};

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

/// Holds the shaders used by the game
struct Shaders {
  GLShader generic_shader;
};

/// Loads all shaders used by the game
Shaders load_shaders()
{
  return {
    .generic_shader = load_generic_shader(),
  };
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GLObjects

/// Vertex representation for a Colored Object
struct ColorVertex {
  glm::vec2 pos;
  glm::vec4 color;
};

/// Vertex representation for a Textured Object
struct TextureVertex {
  glm::vec2 pos;
  glm::vec2 texcoord;
};

/// Represents an object loaded to GPU memory that's renderable using indices
struct GLObject {
  UniqueNum<GLuint> vbo;
  UniqueNum<GLuint> ebo;
  UniqueNum<GLuint> vao;
  size_t num_indices;
  size_t num_vertices;

  ~GLObject() {
    if (vbo) glDeleteBuffers(1, &vbo.inner);
    if (ebo) glDeleteBuffers(1, &ebo.inner);
    if (vao) glDeleteVertexArrays(1, &vao.inner);
  }

  // Movable but not Copyable
  GLObject(GLObject&&) = default;
  GLObject(const GLObject&) = delete;
  GLObject& operator=(GLObject&&) = default;
  GLObject& operator=(const GLObject&) = delete;
};

/// GLObject reference type alias
using GLObjectRef = std::shared_ptr<GLObject>;

/// Upload new Colored Indexed-Vertex object to GPU memory
GLObject create_colored_globject(const GLShader& shader, gsl::span<const ColorVertex> vertices, gsl::span<const GLushort> indices, GLenum usage = GL_STATIC_DRAW)
{
  GLuint vbo, ebo, vao;
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size_bytes(), vertices.data(), usage);
  glEnableVertexAttribArray(shader.attr_loc(GLAttr::POSITION));
  glVertexAttribPointer(shader.attr_loc(GLAttr::POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*) offsetof(ColorVertex, pos));
  glEnableVertexAttribArray(shader.attr_loc(GLAttr::COLOR));
  glVertexAttribPointer(shader.attr_loc(GLAttr::COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*) offsetof(ColorVertex, color));
  glDisableVertexAttribArray(shader.attr_loc(GLAttr::TEXCOORD));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), usage);
  return { vbo, ebo, vao, indices.size(), vertices.size() };
}

/// Upload new Textured Indexed-Vertex object to GPU memory
GLObject create_textured_globject(const GLShader& shader, gsl::span<const TextureVertex> vertices, gsl::span<const GLushort> indices, GLenum usage = GL_STATIC_DRAW)
{
  GLuint vbo, ebo, vao;
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size_bytes(), vertices.data(), usage);
  glEnableVertexAttribArray(shader.attr_loc(GLAttr::POSITION));
  glVertexAttribPointer(shader.attr_loc(GLAttr::POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*) offsetof(TextureVertex, pos));
  glEnableVertexAttribArray(shader.attr_loc(GLAttr::TEXCOORD));
  glVertexAttribPointer(shader.attr_loc(GLAttr::TEXCOORD), 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*) offsetof(TextureVertex, texcoord));
  glDisableVertexAttribArray(shader.attr_loc(GLAttr::COLOR));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), usage);
  return { vbo, ebo, vao, indices.size(), vertices.size() };
}

// Quad Vertices:
// (-1,+1)       (+1,+1)
//  Y ^ - - - - - - o
//    |  D       A  |
//    |   +-----+   |
//    |   | \   |   |
//    |   |  0  |   |
//    |   |   \ |   |
//    |   +-----+   |
//    |  C       B  |
//    o - - - - - - > X
// (-1,-1)       (+1,-1)
// positive Z goes through screen towards you

static constexpr ColorVertex kColorQuadVertices[] = {
  { .pos = { +1.0f, +1.0f }, .color = { 0.0f, 0.0f, 1.0f, 1.0f } },
  { .pos = { +1.0f, -1.0f }, .color = { 0.0f, 1.0f, 0.0f, 1.0f } },
  { .pos = { -1.0f, -1.0f }, .color = { 1.0f, 0.0f, 0.0f, 1.0f } },
  { .pos = { -1.0f, +1.0f }, .color = { 1.0f, 0.0f, 1.0f, 1.0f } },
};

static constexpr TextureVertex kTextureQuadVertices[] = {
  { .pos = { +1.0f, +1.0f }, .texcoord = { 1.0f, 1.0f } },
  { .pos = { +1.0f, -1.0f }, .texcoord = { 1.0f, 0.0f } },
  { .pos = { -1.0f, -1.0f }, .texcoord = { 0.0f, 0.0f } },
  { .pos = { -1.0f, +1.0f }, .texcoord = { 0.0f, 1.0f } },
};

static constexpr GLushort kQuadIndices[] = {
  0, 1, 3,
  1, 2, 3,
};

/// Upload new colored Quad object to GPU memory
GLObject create_colored_quad_globject(const GLShader& shader, GLenum usage = GL_STATIC_DRAW)
{
  return create_colored_globject(shader, kColorQuadVertices, kQuadIndices, usage);
}

/// Upload new textured Quad object to GPU memory
GLObject create_textured_quad_globject(const GLShader& shader, GLenum usage = GL_STATIC_DRAW)
{
  return create_textured_globject(shader, kTextureQuadVertices, kQuadIndices, usage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Sprites

/// Generate quad vertices for a spritesheet texture with frames laid out linearly.
/// count=3:        .texcoord (U,V)
/// (0,1) +-----+-----+-----+ (1,1)
///       |     |     |     |
///       |  1  |  2  |  3  |
///       |     |     |     |
/// (0,0) +-----+-----+-----+ (1,0)
auto gen_sprite_quads(size_t count) -> std::tuple<std::vector<TextureVertex>, std::vector<GLushort>>
{
  float width = 1.0f / count;
  std::vector<TextureVertex> vertices;
  std::vector<GLushort> indices;
  vertices.reserve(4 * count);
  indices.reserve(6 * count);
  for (size_t i = 0; i < count; i++) {
    vertices.emplace_back(TextureVertex{ .pos = { +1.0f, +1.0f }, .texcoord = { (i+1)*width, 1.0f } });
    vertices.emplace_back(TextureVertex{ .pos = { +1.0f, -1.0f }, .texcoord = { (i+1)*width, 0.0f } });
    vertices.emplace_back(TextureVertex{ .pos = { -1.0f, -1.0f }, .texcoord = { (i+0)*width, 0.0f } });
    vertices.emplace_back(TextureVertex{ .pos = { -1.0f, +1.0f }, .texcoord = { (i+0)*width, 1.0f } });
    for (auto v : kQuadIndices)
      indices.emplace_back(4*i+v);
  }
  return {vertices, indices};
}

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Textures

/// Represents a texture loaded to GPU memory
struct GLTexture {
  UniqueNum<GLuint> id;

  ~GLTexture() {
    if (id) glDeleteTextures(1, &id.inner);
  }

  // Movable but not Copyable
  GLTexture(GLTexture&&) = default;
  GLTexture(const GLTexture&) = delete;
  GLTexture& operator=(GLTexture&&) = default;
  GLTexture& operator=(const GLTexture&) = delete;
};

/// GLTexture reference type alias
using GLTextureRef = std::shared_ptr<GLTexture>;

/// Read file and upload RGB/RBGA texture to GPU memory
auto load_rgba_texture(const std::string& inpath, GLenum min_filter, GLenum mag_filter = GLenum(0)) -> std::optional<GLTexture>
{
  const std::string filepath = INMATH_ASSETS_PATH + "/"s + inpath;
  auto file = read_file_to_string(filepath);
  if (!file) { ERROR("Failed to read texture path ({})", filepath); return std::nullopt; }
  int width, height, channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load_from_memory((const uint8_t*)file->data(), file->length(), &width, &height, &channels, 0);
  if (!data) { ERROR("Failed to load texture path ({})", filepath); return std::nullopt; }
  ASSERT_MSG(channels == 4 || channels == 3, "actual channels: {}", channels);
  GLenum type = (channels == 4) ? GL_RGBA : GL_RGB;
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter != GLenum(0) ? mag_filter : min_filter);
  glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data);
  return GLTexture{ texture };
}

/// Holds the textures used by the game
class Textures {
  std::unordered_map<std::string, GLTextureRef> map;

 public:
  /// Get a Texture ID from cache, or load it if not cached yet
  template<typename ...Args>
  auto get_or_load(const std::string& texpath, Args&&... args) -> std::optional<GLTextureRef> {
    auto it = map.find(texpath);
    if (it != map.end()) {
      return it->second;
    } else {
      auto tex = load_rgba_texture(texpath, std::forward<Args>(args)...);
      if (!tex) return std::nullopt;
      auto texptr = std::make_shared<GLTexture>(std::move(*tex));
      map.emplace(texpath, texptr);
      return texptr;
    }
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Fonts

/// Upload font bitmap texture to GPU memory
auto load_font_texture(const uint8_t data[], size_t width, size_t height) -> GLTexture
{
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture); 
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  return { texture };
}

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

/// Holds the fonts used by the game
struct Fonts {
  GLFontRef menlo;
  GLFontRef jetbrains;
  GLFontRef google_sans;
  GLFontRef kanit;
  GLFontRef russo_one;
};

/// Loads all fonts used by the game
Fonts load_fonts()
{
  return {
    .menlo = std::make_shared<GLFont>(*ASSERT_RET(load_font("Menlo-Regular.ttf"))),
    .jetbrains = std::make_shared<GLFont>(*ASSERT_RET(load_font("JetBrainsMono-Regular.ttf"))),
    .google_sans = std::make_shared<GLFont>(*ASSERT_RET(load_font("GoogleSans-Regular.ttf"))),
    .kanit = std::make_shared<GLFont>(*ASSERT_RET(load_font("Kanit/Kanit-Bold.ttf"))),
    .russo_one = std::make_shared<GLFont>(*ASSERT_RET(load_font("Russo_One/RussoOne-Regular.ttf"))),
  };
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Text

/// Generate quad vertices for a text with the given font.
auto gen_text_quads(const GLFont& font, std::string_view text)
{
  std::vector<TextureVertex> vertices;
  vertices.reserve(4 * text.size());
  std::vector<GLushort> indices;
  indices.reserve(6 * text.size());
  float x = 0, y = 0;
  float width = 0;
  size_t i = 0;
  for (char ch : text) {
    if (ch < font.char_beg || ch > (font.char_beg + font.char_count))
      continue;
    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(font.chars.data(), font.bitmap_px_width, font.bitmap_px_height, (int)ch - font.char_beg, &x, &y, &q, 1);
    vertices.emplace_back(TextureVertex{ .pos = { q.x0, q.y0 }, .texcoord = { q.s0, q.t0 } });
    vertices.emplace_back(TextureVertex{ .pos = { q.x1, q.y0 }, .texcoord = { q.s1, q.t0 } });
    vertices.emplace_back(TextureVertex{ .pos = { q.x1, q.y1 }, .texcoord = { q.s1, q.t1 } });
    vertices.emplace_back(TextureVertex{ .pos = { q.x0, q.y1 }, .texcoord = { q.s0, q.t1 } });
    for (auto v : kQuadIndices)
      indices.emplace_back(4*i+v);
    width = q.x1;
    i++;
  }
  return std::tuple{vertices, indices, width};
}

/// Upload new Text Indexed-Vertex object to GPU memory
GLObject create_text_globject(const GLShader& shader, gsl::span<const TextureVertex> vertices, gsl::span<const GLushort> indices, GLenum usage = GL_STATIC_DRAW)
{
  return create_textured_globject(shader, vertices, indices, usage);
}

/// Update GLObject data with the new generated quads for the given Text
auto update_text_globject(const GLShader& shader, GLObject& glo, const GLFont& font, std::string_view text, GLenum usage = GL_STATIC_DRAW)
{
  auto [vertices, indices, _] = gen_text_quads(font, text);
  if (vertices.size() == glo.num_vertices && indices.size() == glo.num_indices) {
    glBindVertexArray(glo.vao);
    glBindBuffer(GL_ARRAY_BUFFER, glo.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(TextureVertex), vertices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glo.ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLushort), indices.data());
  } else {
    glo = create_text_globject(shader, vertices, indices, usage);
  }
}

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
auto load_wav_audio(const std::string& audiopath) -> std::optional<ALBuffer>
{
  DEBUG("Loading audio {}", audiopath);
  const std::string filepath = INMATH_ASSETS_PATH + "/audio/"s + audiopath;
  auto audio = read_file_to_string(filepath);
  if (!audio) { ERROR("Failed to read audio '{}'", audiopath); return std::nullopt; }
  unsigned int channels, samples;
  drwav_uint64 pcm_frame_count;
  drwav_int16* data = drwav_open_memory_and_read_pcm_frames_s16(audio->data(), audio->size(), &channels, &samples, &pcm_frame_count, nullptr);
  if (!data) { ERROR("Failed to load audio path ({})", filepath); return std::nullopt; }
  size_t size = (pcm_frame_count * channels * sizeof(float));
  int err;
  ALuint abo;
  alGenBuffers(1, &abo);
  alBufferData(abo, AL_FORMAT_MONO16, data, size, samples);
  if ((err = alGetError()) != AL_NO_ERROR) {
    ERROR("Failed to buffer audio {}, error {}", audiopath, err);
    alDeleteBuffers(1, &abo);
    drwav_free(data, NULL);
    return std::nullopt;
  } 
  drwav_free(data, NULL);
  return ALBuffer{ abo };
}

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

/// Holds the audio buffers used by the game
struct Audios {
  std::unordered_map<std::string, ALBufferRef> map;

 public:
  /// Get an Audio buffer from cache, or load it if not cached yet
  template<typename ...Args>
  auto get_or_load(const std::string& audiopath, Args&&... args) -> std::optional<ALBufferRef> {
    auto it = map.find(audiopath);
    if (it != map.end()) {
      return it->second;
    } else {
      auto audio = load_wav_audio(audiopath, std::forward<Args>(args)...);
      if (!audio) return std::nullopt;
      auto audioptr = std::make_shared<ALBuffer>(std::move(*audio));
      map.emplace(audiopath, audioptr);
      return audioptr;
    }
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Camera

struct Camera {
  glm::mat4 projection;
  glm::mat4 view;

  /// Create Orthographic Camera
  static Camera create(float aspect_ratio) {
    const float zoom_level = 1.0f;
    const float rotation = 0.0f;
    return {
      .projection = glm::ortho(-aspect_ratio * zoom_level, +aspect_ratio * zoom_level, -zoom_level, +zoom_level, +1.0f, -1.0f),
      .view = glm::inverse(glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0, 0, 1))),
    };
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Renderer

/// Prepare to render
void begin_render()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

/// Upload camera matrix to shader
void set_camera(const GLShader& shader, const Camera& camera)
{
  glUniformMatrix4fv(shader.unif_loc(GLUnif::VIEW), 1, GL_FALSE, glm::value_ptr(camera.view));
  glUniformMatrix4fv(shader.unif_loc(GLUnif::PROJECTION), 1, GL_FALSE, glm::value_ptr(camera.projection));
}

/// Render a colored GLObject with indices
void draw_colored_object(const GLShader& shader, const GLObject& glo, const glm::mat4& model)
{
  if (shader.unif_loc(GLUnif::SUBROUTINE) != -1)
    glUniform1i(shader.unif_loc(GLUnif::SUBROUTINE), static_cast<int>(GLSub::COLOR));
  glUniformMatrix4fv(shader.unif_loc(GLUnif::MODEL), 1, GL_FALSE, glm::value_ptr(model));
  glBindVertexArray(glo.vao);
  glDrawElements(GL_LINE_LOOP, glo.num_indices, GL_UNSIGNED_SHORT, nullptr);
}

/// Render a textured GLObject with indices
void draw_textured_object(const GLShader& shader, const GLTexture& texture, const GLObject& glo,
                          const glm::mat4& model, const SpriteFrame* sprite = nullptr)
{
  if (shader.unif_loc(GLUnif::SUBROUTINE) != -1)
    glUniform1i(shader.unif_loc(GLUnif::SUBROUTINE), static_cast<int>(GLSub::TEXTURE));
  glUniformMatrix4fv(shader.unif_loc(GLUnif::MODEL), 1, GL_FALSE, glm::value_ptr(model));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.id);
  glBindVertexArray(glo.vao);
  size_t ebo_offset = sprite ? sprite->ebo_offset : 0;
  size_t ebo_count = sprite ? sprite->ebo_count : glo.num_indices;
  glDrawElements(GL_TRIANGLES, ebo_count, GL_UNSIGNED_SHORT, (const void*)ebo_offset);
}

/// Render a text GLObject with indices
void draw_text_object(const GLShader& shader, const GLTexture& texture, const GLObject& glo,
                      const glm::mat4& model, const glm::vec4& color, const glm::vec4& outline_color, const float outline_thickness)
{
  if (shader.unif_loc(GLUnif::SUBROUTINE) != -1)
    glUniform1i(shader.unif_loc(GLUnif::SUBROUTINE), static_cast<int>(GLSub::FONT));
  glUniform4fv(shader.unif_loc(GLUnif::COLOR), 1, glm::value_ptr(color));
  glUniform4fv(shader.unif_loc(GLUnif::OUTLINE_COLOR), 1, glm::value_ptr(outline_color));
  glUniform1f(shader.unif_loc(GLUnif::OUTLINE_THICKNESS), outline_thickness);
  glUniformMatrix4fv(shader.unif_loc(GLUnif::MODEL), 1, GL_FALSE, glm::value_ptr(model));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.id);
  glBindVertexArray(glo.vao);
  glDrawElements(GL_TRIANGLES, glo.num_indices, GL_UNSIGNED_SHORT, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward-declarations

static constexpr size_t kWidth = 1600;
static constexpr size_t kHeight = 900;
static constexpr float kAspectRatio = (float)kWidth / (float)kHeight;
static constexpr float kAspectRatioInverse = (float)kHeight / (float)kWidth;

/// GLFW_KEY_*
using Key = int;
/// Key event handler
using KeyHandler = std::function<void(struct Game&, int key, int action, int mods)>;
/// Map key code to event handlers
using KeyHandlerMap = std::unordered_map<Key, KeyHandler>;
/// Map key to its state, pressed = true, released = false
using KeyStateMap = std::unordered_map<Key, bool>;

void init_key_handlers(KeyHandlerMap& key_handlers);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Collision

/// Axis-aligned Bounding Box component
/// in respect to the object's local space
/// (no rotation support)
///     +---+ max
///     | x |
/// min +---+    x = center = origin = transform.position
struct Aabb {
  glm::vec2 min {-1.0f};
  glm::vec2 max {+1.0f};

  Aabb transform(const glm::mat4& matrix) const {
    glm::vec2 a = matrix * glm::vec4(min, 0.0f, 1.0f);
    glm::vec2 b = matrix * glm::vec4(max, 0.0f, 1.0f);
    return Aabb{glm::min(a, b), glm::max(a, b)};
  }
};

/// Check for collision between two AABBs
bool collision(const Aabb& a, const Aabb& b)
{
  return a.min.x < b.max.x &&
         a.max.x > b.min.x &&
         a.min.y < b.max.y &&
         a.max.y > b.min.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Cursor

/// Convert cursor position from window space to normalized space (-1,+1)
///
///  viewport_offset.x
/// |-------|
/// +------------------------+ - viewport_offset.y
/// | (0,0) +-------+ (W, 0) | -
/// |       | view  |        |
/// |       | port  |        |
/// |       | space |        |
/// | (0,H) +-------+ (W, H) |
/// +------------------------+
///       window space
glm::vec2 normalized_cursor_pos(glm::vec2 cursor, glm::uvec2 viewport_size, glm::uvec2 viewport_offset)
{
  float normal_max_width = 2.f * kAspectRatio;
  float normal_max_height = 2.f;
  float x = (((cursor.x - viewport_offset.x) * normal_max_width) / viewport_size.x) - kAspectRatio;
  float y = (((cursor.y - viewport_offset.y) * -normal_max_height) / viewport_size.y) + 1.f;
  return {x, y};
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Components

/// Tag component
struct Tag {
  char label[20] = "?";
};

/// Transform component
struct Transform {
  glm::vec2 position {0.0f};
  glm::vec2 scale    {0.5f};
  float rotation     {0.0f};

  glm::mat4 matrix() {
    glm::mat4 translation_mat = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
    glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(scale, 1.0f));
    return translation_mat * rotation_mat * scale_mat;
  }
};

/// Motion component
struct Motion {
  glm::vec2 velocity     {0.0f};
  glm::vec2 acceleration {0.0f};
};

/// Text Format component
struct TextFormat {
  GLFontRef font;
  glm::vec4 color         {1.0f};
  glm::vec4 outline_color {glm::vec3(0.0f), 1.0f};
  float outline_thickness {1.0f};
};

/// Custom Update Function component
struct UpdateFn {
  std::function<void(struct GameObject& obj, float dt, float time)> fn  { [](auto&&...){} };
};

/// Timed Action component
struct TimedAction {
  float tick_dt;  // deltatime between last round and now
  float duration; // time to go off, in seconds
  std::function<void(Game& game, float dt, float time)> action  { [](auto&&...){} };

  /// Update timer and invoke action on expire
  void update(Game& game, float dt, float time) {
    tick_dt += dt;
    if (tick_dt >= duration) {
      tick_dt -= duration;
      action(game, dt, time);
    }
  }
};

/// Off-Screen Destroy component
struct OffScreenDestroy { };

/// Screen Bound component
struct ScreenBound { };

/// Delay Erasing component
struct DelayErasing {
  bool sound = true;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Game

/// Game Object represents a single Entity with Components
struct GameObject {
  Tag tag;
  Transform transform;
  Transform prev_transform;
  Motion motion;
  GLObjectRef glo;
  std::optional<GLTextureRef> texture;
  std::optional<SpriteAnimation> sprite_animation;
  std::optional<TextFormat> text_fmt;
  std::optional<UpdateFn> update;
  std::optional<Aabb> aabb;
  std::optional<OffScreenDestroy> offscreen_destroy;
  std::optional<ScreenBound> screen_bound;
  std::optional<ALSourceRef> sound;
  std::optional<DelayErasing> delay_erasing;
};

/// Lists of all Game Objects in a Scene, divised in layers, in order of render
struct ObjectLists {
  std::vector<GameObject> background;
  std::vector<GameObject> spaceship;
  std::vector<GameObject> projectile;
  std::vector<GameObject> explosion;
  std::vector<GameObject> gui;
  std::vector<GameObject> text;
  /// Get all layers of objects
  auto all_lists() { return std::array{ &background, &spaceship, &projectile, &explosion, &gui, &text }; }
};

/// Generic Scene structure
struct Scene {
  ObjectLists objects;
  GameObject& player() { return objects.spaceship.front(); }
};

/// Game State/Engine
struct Game {
  bool paused;
  bool vsync;
  bool hover;
  glm::vec2 cursor;
  GLFWwindow* window;
  glm::uvec2 windown_size;
  glm::uvec2 viewport_size;
  glm::uvec2 viewport_offset;
  std::optional<Camera> camera;
  std::optional<Shaders> shaders;
  std::optional<Fonts> fonts;
  std::optional<Scene> scene;
  std::optional<Audios> audios;
  std::optional<Textures> textures;
  std::optional<KeyHandlerMap> key_handlers;
  std::optional<KeyStateMap> key_states;
  std::unordered_map<int, TimedAction> timed_actions;
  Aabb screen_aabb;
  struct {
    bool debug_info = false;
    bool aabbs = false;
  } render_opts;
  struct {
    Transform transform;
    TextFormat text_fmt;
    std::optional<GLObject> glo;
  } fps, obj_counter;
};

/// Create a projectile hit explosion object
GameObject create_explosion(Game& game)
{
  GameObject obj;
  obj.tag = Tag{"explosion"};
  obj.transform = Transform{
    .position = glm::vec2(0.0f),
    .scale = glm::vec2(0.1f),
    .rotation = 0.0f
  };
  obj.prev_transform = obj.transform;
  obj.motion = Motion{
    .velocity = glm::vec2(0.0f),
    .acceleration = glm::vec2(0.0f),
  };
  obj.texture = ASSERT_RET(game.textures->get_or_load("Explosion.png", GL_LINEAR));
  auto [vertices, indices] = gen_sprite_quads(6);
  obj.glo = std::make_shared<GLObject>(create_textured_globject(game.shaders->generic_shader, vertices, indices));
  obj.sprite_animation = SpriteAnimation{
    .last_transit_dt = 0,
    .curr_frame_idx = 0,
    .frames = std::vector<SpriteFrame>{
      { .duration = 0.04f, .ebo_offset = 00, .ebo_count = 6 },
      { .duration = 0.04f, .ebo_offset = 12, .ebo_count = 6 },
      { .duration = 0.04f, .ebo_offset = 24, .ebo_count = 6 },
      { .duration = 0.04f, .ebo_offset = 36, .ebo_count = 6 },
      { .duration = 0.04f, .ebo_offset = 48, .ebo_count = 6 },
      { .duration = 0.06f, .ebo_offset = 60, .ebo_count = 6 },
    },
    .curr_cycle_count = 0,
    .max_cycles = 1,
  };
  obj.sound = std::make_shared<ALSource>(create_audio_source(1.0f));
  obj.sound->get()->bind_buffer(*ASSERT_RET(game.audios->get_or_load("explosionCrunch_000.wav")));
  return obj;
}

/// Create a player spaceship fired projectile object
GameObject create_player_projectile(Game& game)
{
  GameObject obj;
  obj.tag = Tag{"projectile"};
  obj.transform = Transform{
    .position = glm::vec2(0.0f, 0.0f),
    .scale = glm::vec2(0.15f),
    .rotation = 0.0f
  };
  obj.prev_transform = obj.transform;
  obj.motion = Motion{
    .velocity = glm::vec2(0.0f, 2.6f),
    .acceleration = glm::vec2(0.0f),
  };
  obj.texture = ASSERT_RET(game.textures->get_or_load("Projectile01.png", GL_LINEAR));
  obj.glo = std::make_shared<GLObject>(create_textured_quad_globject(game.shaders->generic_shader));
  obj.aabb = Aabb{ .min= {-0.11f, -0.38f}, .max = {+0.07f, +0.30f} };
  obj.offscreen_destroy = OffScreenDestroy{};
  obj.sound = std::make_shared<ALSource>(create_audio_source(0.8f));
  obj.sound->get()->bind_buffer(*ASSERT_RET(game.audios->get_or_load("laser-14729.wav")));
  return obj;
}

int game_init(Game& game, GLFWwindow* window)
{
  INFO("Initializing game");
  game.paused = false;
  game.vsync = true;
  game.hover = false;
  game.cursor = glm::vec2(0.f);
  game.window = window;
  game.windown_size = glm::uvec2(kWidth, kHeight);
  game.viewport_size = glm::uvec2(kWidth, kHeight);
  game.viewport_offset = glm::uvec2(0);
  game.camera = Camera::create(kAspectRatio);
  game.shaders = load_shaders();
  game.fonts = load_fonts();
  game.scene = Scene{};
  game.audios = Audios{};
  game.textures = Textures{};
  game.key_handlers = KeyHandlerMap(GLFW_KEY_LAST); // reserve all keys to avoid rehash
  game.key_states = KeyStateMap(GLFW_KEY_LAST);     // reserve all keys to avoid rehash
  game.screen_aabb = Aabb{ .min = {-kAspectRatio, -1.0f}, .max = {kAspectRatio, +1.0f} };

  ASSERT(game.audios->get_or_load("laser-14729.wav"));
  ASSERT(game.audios->get_or_load("explosionCrunch_000.wav"));

  { // Background
    game.scene->objects.background.push_back({});
    GameObject& background = game.scene->objects.background.back();
    background.tag = Tag{"background"};
    background.transform = Transform{
      .position = glm::vec2(0.0f),
      .scale = glm::vec2(kAspectRatio + 0.1f, 1.1f),
      .rotation = 0.0f,
    };
    background.prev_transform = background.transform;
    background.motion = Motion{
      .velocity = glm::vec2(0.014f, 0.004f),
      .acceleration = glm::vec2(0.0f),
    };
    DEBUG("Loading Background Texture");
    background.texture = ASSERT_RET(game.textures->get_or_load("background01.png", GL_NEAREST));
    DEBUG("Loading Background Quad");
    background.glo = std::make_shared<GLObject>(create_textured_quad_globject(game.shaders->generic_shader));
    background.update = UpdateFn{ [] (struct GameObject& obj, float dt, float time) {
      if (obj.transform.position.x < -0.03f || obj.transform.position.x >= +0.03f)
        obj.motion.velocity.x = -obj.motion.velocity.x;
      if (obj.transform.position.y < -0.03f || obj.transform.position.y >= +0.03f)
        obj.motion.velocity.y = -obj.motion.velocity.y;
    }};
  }

  { // Player
    game.scene->objects.spaceship.push_back({});
    GameObject& player = game.scene->objects.spaceship.back();
    player.tag = Tag{"player"};
    player.transform = Transform{
      .position = glm::vec2(0.0f, -0.7f),
      .scale = glm::vec2(0.1f),
      .rotation = 0.0f,
    };
    player.prev_transform = player.transform;
    player.motion = Motion{
      .velocity = glm::vec2(0.0f),
      .acceleration = glm::vec2(0.0f),
    };
    DEBUG("Loading Player Spaceship Texture");
    player.texture = ASSERT_RET(game.textures->get_or_load("Lightning.png", GL_NEAREST));
    DEBUG("Loading Player Spaceship Vertices");
    auto [vertices, indices] = gen_sprite_quads(4);
    player.glo = std::make_shared<GLObject>(create_textured_globject(game.shaders->generic_shader, vertices, indices));
    DEBUG("Loading Player Spaceship Sprite Animation");
    player.sprite_animation = SpriteAnimation{
      .last_transit_dt = 0,
      .curr_frame_idx = 0,
      .frames = std::vector<SpriteFrame>{
        { .duration = 0.15, .ebo_offset = 00, .ebo_count = 6 },
        { .duration = 0.15, .ebo_offset = 12, .ebo_count = 6 },
        { .duration = 0.15, .ebo_offset = 24, .ebo_count = 6 },
        { .duration = 0.15, .ebo_offset = 36, .ebo_count = 6 },
      },
      .curr_cycle_count = 0,
      .max_cycles = 0,
    };
    player.aabb = Aabb{ .min = {-0.80f, -0.70f}, .max = {0.82f, 0.70f} };
    player.screen_bound = ScreenBound{};
  }

  { // Enemy
    game.scene->objects.spaceship.push_back({});
    GameObject& enemy = game.scene->objects.spaceship.back();
    enemy.tag = Tag{"enemy"};
    enemy.transform = Transform{
      .position = glm::vec2(0.0f, +0.5f),
      .scale = glm::vec2(0.08f, -0.08f),
      .rotation = 0.0f
    };
    enemy.prev_transform = enemy.transform;
    enemy.motion = Motion{
      .velocity = glm::vec2(0.0f),
      .acceleration = glm::vec2(0.0f),
    };
    DEBUG("Loading Enemy Spaceship Texture");
    enemy.texture = ASSERT_RET(game.textures->get_or_load("Saboteur.png", GL_NEAREST));
    DEBUG("Loading Enemy Spaceship Vertices");
    auto [vertices, indices] = gen_sprite_quads(4);
    enemy.glo = std::make_shared<GLObject>(create_textured_globject(game.shaders->generic_shader, vertices, indices));
    DEBUG("Loading Enemy Spaceship Sprite Animation");
    enemy.sprite_animation = SpriteAnimation{
      .last_transit_dt = 0,
      .curr_frame_idx = 0,
      .frames = std::vector<SpriteFrame>{
        { .duration = 0.15, .ebo_offset = 00, .ebo_count = 6 },
        { .duration = 0.15, .ebo_offset = 12, .ebo_count = 6 },
        { .duration = 0.15, .ebo_offset = 24, .ebo_count = 6 },
        { .duration = 0.15, .ebo_offset = 36, .ebo_count = 6 },
      },
      .curr_cycle_count = 0,
      .max_cycles = 0,
    };
    enemy.update = UpdateFn{ [] (struct GameObject& obj, float dt, float time) {
      obj.transform.position.x = std::sin(time) * 0.4f;
    }};
    enemy.aabb = Aabb{ .min = {-0.55f, -0.50f}, .max = {0.55f, 0.50f} };
  };

  { // FPS
    auto& fps = game.fps;
    fps.transform = Transform{};
    fps.transform.position = glm::vec2(-0.99f * kAspectRatio, -0.99f);
    fps.transform.scale = glm::vec2(0.0024f);
    fps.transform.scale.y = -fps.transform.scale.y;
    DEBUG("Loading FPS Text");
    auto [vertices, indices, _] = gen_text_quads(*game.fonts->russo_one, "FPS 00 ms 00.000");
    fps.glo = create_text_globject(game.shaders->generic_shader, vertices, indices, GL_DYNAMIC_DRAW);
    fps.text_fmt = TextFormat{
      .font = game.fonts->russo_one,
      .color = kWhiteDimmed,
      .outline_color = kBlack,
      .outline_thickness = 1.0f,
    };
  }

  { // OBJ Counter
    auto& obj = game.obj_counter;
    obj.transform = Transform{};
    obj.transform.position = glm::vec2(0.68f * kAspectRatio, -0.99f);
    obj.transform.scale = glm::vec2(0.0024f);
    obj.transform.scale.y = -obj.transform.scale.y;
    DEBUG("Loading OBJ Counter Text");
    auto [vertices, indices, _] = gen_text_quads(*game.fonts->russo_one, "OBJ 000");
    obj.glo = create_text_globject(game.shaders->generic_shader, vertices, indices, GL_DYNAMIC_DRAW);
    obj.text_fmt = TextFormat{
      .font = game.fonts->russo_one,
      .color = kWhiteDimmed,
      .outline_color = kBlack,
      .outline_thickness = 1.0f,
    };
  }

  return 0;
}

void game_resume(Game& game)
{
  if (!game.paused) return;
  INFO("Resuming game");
  game.paused = false;
}

void game_pause(Game& game)
{
  if (game.paused) return;
  INFO("Pausing game");

  // Release all active keys
  for (auto& key_state : game.key_states.value()) {
    auto& key = key_state.first;
    auto& active = key_state.second;
    if (active) {
      active = false;
      auto it = game.key_handlers->find(key);
      if (it != game.key_handlers->end()) {
        auto& handler = it->second;
        handler(game, key, GLFW_RELEASE, 0);
      }
    }
  }

  game.paused = true;
}

void game_update(Game& game, float dt, float time)
{
  // Update TimedAction's
  for (auto& timed_action : game.timed_actions) {
    timed_action.second.update(game, dt, time);
  }

  // Update Erasing system
  for (auto* object_list : game.scene->objects.all_lists()) {
    for (auto&& obj = object_list->begin(); obj != object_list->end();) {
      if (obj->delay_erasing) {
        bool erase = true;
        if (obj->delay_erasing->sound && obj->sound) {
          ALint state = 0;
          alGetSourcei(obj->sound->get()->id, AL_SOURCE_STATE, &state);
          if (state == AL_PLAYING)
            erase = false;
        }
        if (erase) {
          obj = object_list->erase(obj);
          continue;
        }
      }
      obj++;
    }
  }

  // Cursor Picking system
  {
    game.hover = false;
    const auto pixel_size = glm::vec2(1.f / game.viewport_size.x, 1.f / game.viewport_size.y);
    const auto cursor_pos = normalized_cursor_pos(game.cursor, game.viewport_size, game.viewport_offset);
    const auto cursor_aabb = Aabb{.min = cursor_pos, .max = cursor_pos + pixel_size};
    for (auto &spaceship : game.scene->objects.spaceship) {
      if (!spaceship.aabb) continue;
      Aabb spaceship_aabb = spaceship.aabb->transform(spaceship.transform.matrix());
      if (collision(cursor_aabb, spaceship_aabb)) {
        game.hover = true;
      }
    }
  }

  // Update all objects
  for (auto* object_list : game.scene->objects.all_lists()) {
    for (auto& obj : *object_list) {
      // Transform
      obj.prev_transform = obj.transform;
      // Motion system
      obj.motion.velocity += obj.motion.acceleration * dt;
      obj.transform.position += obj.motion.velocity * dt;
      // Sprite Animation system
      if (obj.sprite_animation) {
        obj.sprite_animation->update_frame(dt);
        if (obj.sprite_animation->expired()) {
          if (!obj.delay_erasing) {
            obj.delay_erasing = DelayErasing{.sound = true};
            obj.transform = Transform{
                .position = glm::vec2(1000.0f),
            };
            obj.prev_transform = obj.transform;
          }
        }
      }
      // Custom Update Function system
      if (obj.update) obj.update->fn(obj, dt, time);
      // Off-Screen Destroy system
      if (obj.offscreen_destroy) {
        Aabb obj_aabb = obj.aabb->transform(obj.transform.matrix());
        if (!collision(obj_aabb, game.screen_aabb)) {
          if (!obj.delay_erasing)
            obj.delay_erasing = DelayErasing{};
        }
      }
      // Screen Bound system
      if (obj.screen_bound) {
        glm::vec2& pos = obj.transform.position;
        if (pos.x < game.screen_aabb.min.x) pos.x = game.screen_aabb.min.x;
        if (pos.y < game.screen_aabb.min.y) pos.y = game.screen_aabb.min.y;
        if (pos.x > game.screen_aabb.max.x) pos.x = game.screen_aabb.max.x;
        if (pos.y > game.screen_aabb.max.y) pos.y = game.screen_aabb.max.y;
      }
    }
  }

  // Projectile<->Spaceship Collision system
  {
    auto& spaceships = game.scene->objects.spaceship;
    auto&& spaceship = spaceships.begin();
    for (spaceship++ /* skip player */; spaceship != spaceships.end(); spaceship++) {
      auto& projectiles = game.scene->objects.projectile;
      for (auto& projectile : projectiles) {
        Aabb projectile_aabb = projectile.aabb->transform(projectile.transform.matrix());
        Aabb spaceship_aabb = spaceship->aabb->transform(spaceship->transform.matrix());
        if (collision(projectile_aabb, spaceship_aabb)) {
          GameObject explosion = create_explosion(game);
          explosion.transform.position = projectile.transform.position;
          explosion.prev_transform = explosion.transform;
          explosion.sound->get()->play();
          game.scene->objects.explosion.emplace_back(std::move(explosion));
          if (!projectile.delay_erasing) {
            projectile.delay_erasing = DelayErasing{.sound = true};
            projectile.transform = Transform{
              .position = glm::vec2(1000.0f),
            };
            projectile.prev_transform = projectile.transform;
          }
        }
      }
    }
  }

  // Player<->Enemy Spaceships Collision system
  if (!game.paused)
  {
    auto& spaceships = game.scene->objects.spaceship;
    auto&& player = spaceships.begin();
    auto&& enemy = spaceships.begin();
    for (enemy++ /* skip player */; enemy != spaceships.end(); enemy++) {
      Aabb player_aabb = player->aabb->transform(player->transform.matrix());
      Aabb enemy_aabb = enemy->aabb->transform(enemy->transform.matrix());
      if (collision(player_aabb, enemy_aabb)) {
        GameObject explosion = create_explosion(game);
        explosion.transform.position = player->transform.position;
        explosion.prev_transform = explosion.transform;
        explosion.sound->get()->play();
        game.scene->objects.explosion.emplace_back(std::move(explosion));
        player->glo = nullptr;
        game_pause(game);
      }
    }
  }
}

/// Calculates the average FPS within kPeriod and update FPS GLObject data for render
void update_fps(const GLShader& shader, GLObject& glo, const GLFont& font, float dt)
{
  constexpr float kPeriod = 0.3f; // second
  static size_t counter = 1;
  counter++;
  float fps_now = (1.f / dt);
  static float fps_avg = fps_now;
  fps_avg += fps_now;
  static float fps_dt = 0;
  fps_dt += dt;
  static float fps = fps_avg;
  if (fps_dt > kPeriod) {
    fps_dt -= kPeriod;
    fps = fps_avg / counter;
    fps_avg = fps_now;
    counter = 1;
  }
  static float last_fps = 0;
  if (fps != last_fps) {
    last_fps = fps;
    char fps_cbuf[30];
    float ms = (1.f / fps) * 1000;
    std::snprintf(fps_cbuf, sizeof(fps_cbuf), "FPS %.0f ms %.3f", fps, ms);
    update_text_globject(shader, glo, font, fps_cbuf, GL_DYNAMIC_DRAW);
  }
}

/// Compute number of objects being rendered and update OBJ Counter GLObject for render
void update_obj_counter(Game& game, const GLShader& shader, GLObject& glo, const GLFont& font)
{
  static size_t last_obj_counter = 0;
  size_t obj_counter = 0;
  for (auto* object_list : game.scene->objects.all_lists())
    obj_counter += object_list->size();
  if (obj_counter != last_obj_counter) {
    last_obj_counter = obj_counter;
    char obj_cbuf[30];
    std::snprintf(obj_cbuf, sizeof(obj_cbuf), "OBJ %03zu", obj_counter);
    update_text_globject(shader, glo, font, obj_cbuf, GL_DYNAMIC_DRAW);
  }
}

/// Render a text in immediate mode: construct text object, draw and discard
void immediate_draw_text(const GLShader &shader, const std::string_view text, const std::optional<glm::vec2> position, const GLFont &font,
                         const float text_size_px, const glm::vec4 &color, const glm::vec4 &outline_color, const float outline_thickness)
{
  const auto [vertices, indices, width] = gen_text_quads(font, text);
  auto glo = create_text_globject(shader, vertices, indices, GL_STREAM_DRAW);
  const float normal_pixel_scale = 1.f / font.pixel_height;
  const float normal_text_scale = text_size_px / kHeight;
  float scale = normal_pixel_scale * normal_text_scale;
  auto transform = Transform{};
  transform.scale = glm::vec2(scale);
  transform.scale.y = -transform.scale.y;
  if (position) transform.position = *position;
  else /*center*/ transform.position.x -= transform.scale.x * (width / 2.f);
  draw_text_object(shader, font.texture, glo, transform.matrix(), kWhite, kBlack, 1.f);
}

/// Render AABBs for all objects that have it
void render_aabbs(Game& game, GLShader& generic_shader)
{
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  auto bbox_glo = create_colored_globject(generic_shader, kColorQuadVertices, kQuadIndices, GL_STREAM_DRAW);
  GLushort indices[] = {0, 1, 2, 3};
  glBindVertexArray(bbox_glo.vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bbox_glo.ebo);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);
  bbox_glo.num_indices = 4;
  for (auto* object_list : game.scene->objects.all_lists()) {
    for (auto obj = object_list->rbegin(); obj != object_list->rend(); obj++) {
      if (obj->aabb) {
        Aabb& aabb = *obj->aabb;
        auto vertices = std::vector<ColorVertex>{
          { .pos = { aabb.max.x, aabb.max.y }, .color = { 1.0f, 1.0f, 0.0f, 1.0f } },
          { .pos = { aabb.max.x, aabb.min.y }, .color = { 1.0f, 1.0f, 0.0f, 1.0f } },
          { .pos = { aabb.min.x, aabb.min.y }, .color = { 1.0f, 1.0f, 0.0f, 1.0f } },
          { .pos = { aabb.min.x, aabb.max.y }, .color = { 1.0f, 1.0f, 0.0f, 1.0f } },
        };
        //glBindVertexArray(bbox_glo.vao);
        glBindBuffer(GL_ARRAY_BUFFER, bbox_glo.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(ColorVertex), vertices.data());
        draw_colored_object(generic_shader, bbox_glo, obj->transform.matrix());
      }
    }
  }
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void game_render(Game& game, float frame_time, float alpha)
{
  begin_render();

  GLShader& generic_shader = game.shaders->generic_shader;
  generic_shader.bind();
  set_camera(generic_shader, *game.camera);

  // Render all objects
  for (auto* object_list : game.scene->objects.all_lists()) {
    for (auto obj = object_list->rbegin(); obj != object_list->rend() && obj->glo; obj++) {
      // Linear interpolation
      auto transform = Transform{
        .position = glm::lerp(obj->prev_transform.position, obj->transform.position, alpha),
        .scale = glm::lerp(obj->prev_transform.scale, obj->transform.scale, alpha),
        .rotation = glm::lerp(obj->prev_transform.rotation, obj->transform.rotation, alpha),
      };
      // Draw object
      if (obj->texture) {
        if (obj->sprite_animation) {
          SpriteFrame& frame = obj->sprite_animation->curr_frame();
          draw_textured_object(generic_shader, *obj->texture->get(), *obj->glo, transform.matrix(), &frame);
        } else {
          draw_textured_object(generic_shader, *obj->texture->get(), *obj->glo, transform.matrix());
        }
      }
      else if (obj->text_fmt) {
        draw_text_object(generic_shader, obj->text_fmt->font->texture, *obj->glo, transform.matrix(),
                         obj->text_fmt->color, obj->text_fmt->outline_color, obj->text_fmt->outline_thickness);
      }
      else {
        draw_colored_object(generic_shader, *obj->glo, transform.matrix());
      }
    }
  }

  // Render AABBs
  if (game.render_opts.aabbs && game.hover)
    render_aabbs(game, generic_shader);

  // Render Debug Info
  if (game.render_opts.debug_info) {
    auto& fps = game.fps;
    update_fps(generic_shader, *fps.glo, *fps.text_fmt.font, frame_time);
    draw_text_object(generic_shader, fps.text_fmt.font->texture, *fps.glo, fps.transform.matrix(),
                     fps.text_fmt.color, fps.text_fmt.outline_color, fps.text_fmt.outline_thickness);

    auto& objc = game.obj_counter;
    update_obj_counter(game, generic_shader, *objc.glo, *objc.text_fmt.font);
    draw_text_object(generic_shader, objc.text_fmt.font->texture, *objc.glo, objc.transform.matrix(),
                     objc.text_fmt.color, objc.text_fmt.outline_color, objc.text_fmt.outline_thickness);
  }

  // Render Game Pause
  if (game.paused)
    immediate_draw_text(generic_shader, "PAUSED", std::nullopt, *game.fonts->russo_one, 50.f, kWhite, kBlack, 1.f);

  // Render Cursor
  //auto cursor_pos = normalized_cursor_pos(game.cursor, game.winsize);
  //auto cursor_obj = create_colored_quad_globject(generic_shader);
  //auto transform = Transform{};
  //transform.scale = glm::vec2(0.03f);
  //transform.position = glm::vec2(cursor_pos.x, cursor_pos.y);
  //draw_colored_object(generic_shader, cursor_obj, transform.matrix());
}

int game_loop(GLFWwindow* window)
{
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    begin_render();
    glfwSwapBuffers(window);
  }
  return 0;

  Game game;
  int ret = game_init(game, window);
  if (ret) return ret;
  init_key_handlers(*game.key_handlers);
  glfwSetWindowUserPointer(window, &game);
  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(monitor);
  const float refresh_rate = mode->refreshRate;

  float epochtime = 0;
  float last_time = 0;
  float update_lag = 0;
  float render_lag = 0;
  constexpr float timestep = 1.f / 100.f;

  while (!glfwWindowShouldClose(window)) {
    float now_time = glfwGetTime();
    float loop_time = now_time - last_time;
    last_time = now_time;

    update_lag += loop_time;
    while (update_lag >= timestep) {
      glfwPollEvents();
      game_update(game, timestep, epochtime);
      epochtime += timestep;
      update_lag -= timestep;
    }

    render_lag += loop_time;
    const float render_interval = game.vsync ? (1.f / (refresh_rate + 0.5f)) : 0.0f;
    if (render_lag >= render_interval) {
      float alpha = update_lag / timestep;
      game_render(game, render_lag, alpha);
      glfwSwapBuffers(window);
      render_lag = 0;
    }

    float next_loop_time_diff_us = std::min(timestep - update_lag, render_interval - render_lag);
    next_loop_time_diff_us *= 1'000'000.f;
    if (next_loop_time_diff_us > 10.f)
      usleep(next_loop_time_diff_us / 2.f);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Event Handlers

void key_left_right_handler(struct Game& game, int key, int action, int mods)
{
  ASSERT(key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT);
  const float direction = (key == GLFW_KEY_LEFT ? -1.f : +1.f);
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    game.scene->player().motion.velocity.x = 0.5f * direction;
    game.scene->player().motion.acceleration.x = 1.8f * direction;
  }
  else if (action == GLFW_RELEASE) {
    const int other_key = (key == GLFW_KEY_LEFT) ? GLFW_KEY_RIGHT : GLFW_KEY_LEFT;
    if (game.key_states.value()[other_key]) {
      // revert to opposite key's movement
      game.key_handlers.value()[other_key](game, other_key, GLFW_REPEAT, mods);
    } else {
      // both arrow keys release, cease movement
      game.scene->player().motion.velocity.x = 0.0f;
      game.scene->player().motion.acceleration.x = 0.0f;
    }
  }
}

void key_up_down_handler(struct Game& game, int key, int action, int mods)
{
  ASSERT(key == GLFW_KEY_UP || key == GLFW_KEY_DOWN);
  const float direction = (key == GLFW_KEY_UP ? +1.f : -1.f);
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    game.scene->player().motion.velocity.y = 0.5f * direction;
    game.scene->player().motion.acceleration.y = 1.2f * direction;
  }
  else if (action == GLFW_RELEASE) {
    const int other_key = (key == GLFW_KEY_UP) ? GLFW_KEY_DOWN : GLFW_KEY_UP;
    if (game.key_states.value()[other_key]) {
      // revert to opposite key's movement
      game.key_handlers.value()[other_key](game, other_key, GLFW_REPEAT, mods);
    } else {
      // both arrow keys release, cease movement
      game.scene->player().motion.velocity.y = 0.0f;
      game.scene->player().motion.acceleration.y = 0.0f; 
    }
  }
}

void key_space_handler(struct Game& game, int key, int action, int mods)
{
  ASSERT(key == GLFW_KEY_SPACE);
  if (action == GLFW_PRESS) {
    auto spawn_projectile = [] (Game& game) {
      GameObject& player = game.scene->player();
      GameObject projectile = create_player_projectile(game);
      constexpr glm::vec2 offset = { 0.062f, 0.125f };
      projectile.transform.position = player.transform.position;
      projectile.transform.position.x -= offset.x;
      projectile.transform.position.x += 0.005f; // sprite correction
      projectile.transform.position.y += offset.y;
      projectile.prev_transform = projectile.transform;
      game.scene->objects.projectile.emplace_back(projectile);
      projectile.transform.position = player.transform.position;
      projectile.transform.position.x += offset.x;
      projectile.transform.position.y += offset.y;
      projectile.prev_transform = projectile.transform;
      game.scene->objects.projectile.emplace_back(std::move(projectile));
      game.scene->objects.projectile.back().sound->get()->play();
    };
    spawn_projectile(game);
    game.timed_actions[0] = TimedAction {
      .tick_dt = 0,
      .duration = 0.150f,
      .action = [=] (Game& game, auto&&...) { spawn_projectile(game); },
    };
  } else if (action == GLFW_RELEASE) {
    game.timed_actions.erase(0);
  }
}

void key_f3_handler(struct Game& game, int key, int action, int mods)
{
  if (action == GLFW_PRESS)
    game.render_opts.debug_info = !game.render_opts.debug_info;
}

void key_f7_handler(struct Game& game, int key, int action, int mods)
{
  if (action == GLFW_PRESS)
    game.render_opts.aabbs = !game.render_opts.aabbs;
}

void key_f6_handler(struct Game& game, int key, int action, int mods)
{
  if (action == GLFW_PRESS)
    game.vsync = !game.vsync;
}

void init_key_handlers(KeyHandlerMap& key_handlers)
{
  key_handlers[GLFW_KEY_LEFT] = key_left_right_handler;
  key_handlers[GLFW_KEY_RIGHT] = key_left_right_handler;
  key_handlers[GLFW_KEY_UP] = key_up_down_handler;
  key_handlers[GLFW_KEY_DOWN] = key_up_down_handler;
  key_handlers[GLFW_KEY_SPACE] = key_space_handler;
  key_handlers[GLFW_KEY_F3] = key_f3_handler;
  key_handlers[GLFW_KEY_F6] = key_f6_handler;
  key_handlers[GLFW_KEY_F7] = key_f7_handler;
}

void key_event_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (!game) return;

  if (action != GLFW_PRESS && action != GLFW_RELEASE)
    return;

  TRACE("Event key: {} action: {} mods: {}", key, action, mods);

  auto it = game->key_handlers->find(key);
  if (it != game->key_handlers->end()) {
    auto& handler = it->second;
    handler(*game, key, action, mods);
  }
  game->key_states.value()[key] = (action == GLFW_PRESS);
}

void window_focus_callback(GLFWwindow* window, int focused)
{
  auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (!game) return;

  if (focused) {
    DEBUG("Window Focused");
    game_resume(*game);
  } else /* unfocused */ {
    DEBUG("Window Unfocused");
    game_pause(*game);
  }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (!game) return;

  float x_rest = 0.0f;
  float y_rest = 0.0f;
  float aspect = ((float)width / (float)height);
  if (aspect < kAspectRatio)
    y_rest = height - (width * kAspectRatioInverse);
  else
    x_rest = width - (height * kAspectRatio);
  float x_off = x_rest / 2.f;
  float y_off = y_rest / 2.f;
  glViewport(x_off, y_off, (width - x_rest), (height - y_rest));

  game->windown_size.y = height;
  game->windown_size.x = width;
  game->viewport_size.x = (width - x_rest);
  game->viewport_size.y = (height - y_rest);
  game->viewport_offset.x = x_off;
  game->viewport_offset.y = y_off;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (!game) return;
  game->cursor.x = (float)xpos;
  game->cursor.y = (float)ypos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Setup

/// Initialize OpenAL
int init_audio(ALCcontext *context)
{
  int err;
  bool ok;
  ALCdevice *device = alcOpenDevice(nullptr);
  if (!device) {
    CRITICAL("Failed to open default audio device");
    return -4;
  }

  context = alcCreateContext(device, nullptr);
  ok = alcMakeContextCurrent(context);
  if (!ok || (err = alGetError()) != AL_NO_ERROR) {
    CRITICAL("Failed to create OpenAL context");
    return -4;
  }

  ALfloat orientation[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
  alListener3f(AL_POSITION, 0.0f, 0.0f, 1.0f);
  alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
  alListenerfv(AL_ORIENTATION, orientation);
  if ((err = alGetError()) != AL_NO_ERROR) {
    CRITICAL("Failed to configure OpenAL Listener");
    return -4;
  }

  return 0;
};

/// Create a window with GLFW
int create_window(GLFWwindow*& window)
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(kWidth, kHeight, "inmath", nullptr, nullptr);
  if (window == nullptr) {
    CRITICAL("Failed to create GLFW window");
    glfwTerminate();
    return -3;
  }
  glfwMakeContextCurrent(window);

  // callbacks
  glfwSetKeyCallback(window, key_event_callback);
  glfwSetWindowFocusCallback(window, window_focus_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);

  // settings
  glfwSetWindowAspectRatio(window, kWidth, kHeight);
  glfwSwapInterval(0);

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main

int main(int argc, char *argv[])
{
  int ret = 0;
  auto log_level = spdlog::level::info;

  // Parse Arguments ===========================================================
  for (int argi = 1; argi < argc; ++argi) {
    if (!strcmp(argv[argi], "--log")) {
      argi++;
      if (argi < argc) {
        log_level = spdlog::level::from_str(argv[argi]);
      } else {
        fprintf(stderr, "--log: missing argument\n");
        return -2;
      }
    } else {
      fprintf(stderr, "unknown argument: %s\n", argv[argi]);
      return -1;
    }
  }

  // Setup Logging =============================================================
  spdlog::set_default_logger(spdlog::stdout_color_mt("main"));
  spdlog::set_pattern("%Y-%m-%d %T.%e <%^%l%$> [%n] %s:%#: %!() -> %v");
  spdlog::set_level(log_level);
  INFO("Initializing..");

  // Create Window =============================================================
  INFO("Creating Window..");
  GLFWwindow* window = nullptr;
  ret = create_window(window);
  if (ret) return ret;

  // Load GL ===================================================================
  INFO("Loading OpenGL..");
  glbinding::initialize(glfwGetProcAddress);
  glbinding::aux::enableGetErrorCallback();

  // Init AL ===================================================================
  INFO("Initializing OpenAL..");
  ALCcontext* openal_ctx = nullptr;
  ret = init_audio(openal_ctx);
  if (ret) { glfwTerminate(); return ret; }

  // Game Loop =================================================================
  INFO("Game Loop..");
  ret = game_loop(window);

  // End =======================================================================
  INFO("Terminating..");
  ALCdevice *alc_device = alcGetContextsDevice(openal_ctx);
  alcMakeContextCurrent(NULL);
  alcDestroyContext(openal_ctx);
  alcCloseDevice(alc_device);
  glfwTerminate();

  INFO("Exit");
  return ret;
}
