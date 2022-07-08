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
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <yaml-cpp/yaml.h>

#include "core/log.hpp"
#include "core/file.hpp"
#include "./colors.hpp"
#include "core/unique_num.hpp"
#include "core/gl_object.hpp"
#include "core/aabb.hpp"
#include "core/sprite.hpp"
#include "core/text.hpp"
#include "./fonts.hpp"
#include "core/cursor.hpp"
#include "core/camera.hpp"
#include "./audios.hpp"
#include "core/window.hpp"
#include "./shaders.hpp"
#include "core/viewport.hpp"
#include "./textures.hpp"
#include "core/renderer.hpp"
#include "./components.hpp"

using namespace std::string_literals;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward-declarations

static constexpr size_t kWidth = 1280;
static constexpr size_t kHeight = 720;
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
  std::optional<Health> health;
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
  Cursor cursor;
  Window window;
  Viewport viewport;
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
  obj.texture = ASSERT_GET(game.textures->get("Explosion.png"));
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
  obj.sound->get()->bind_buffer(*ASSERT_GET(game.audios->get("explosionCrunch_000.wav")));
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
  obj.texture = ASSERT_GET(game.textures->get("Projectile01.png"));
  obj.glo = std::make_shared<GLObject>(create_textured_quad_globject(game.shaders->generic_shader));
  obj.aabb = Aabb{ .min= {-0.11f, -0.38f}, .max = {+0.07f, +0.30f} };
  obj.offscreen_destroy = OffScreenDestroy{};
  obj.sound = std::make_shared<ALSource>(create_audio_source(0.8f));
  obj.sound->get()->bind_buffer(*ASSERT_GET(game.audios->get("laser-14729.wav")));
  return obj;
}

int game_init(Game& game, GLFWwindow* window)
{
  INFO("Initializing game");
  game.paused = false;
  game.vsync = true;
  game.hover = false;
  game.cursor = Cursor();
  game.window.glfw = window;
  game.window.size = glm::uvec2(kWidth, kHeight);
  game.viewport.size = glm::uvec2(kWidth, kHeight);
  game.viewport.offset = glm::uvec2(0);
  game.camera = Camera::create(kAspectRatio);
  game.shaders = load_shaders();
  game.fonts = load_fonts();
  game.scene = Scene{};
  game.audios = Audios{};
  game.textures = Textures{};
  game.key_handlers = KeyHandlerMap(GLFW_KEY_LAST); // reserve all keys to avoid rehash
  game.key_states = KeyStateMap(GLFW_KEY_LAST);     // reserve all keys to avoid rehash
  game.screen_aabb = Aabb{ .min = {-kAspectRatio, -1.0f}, .max = {kAspectRatio, +1.0f} };

  ASSERT(game.audios->load("laser-14729.wav"));
  ASSERT(game.audios->load("explosionCrunch_000.wav"));

  ASSERT(game.textures->load("Explosion.png", GL_LINEAR));
  ASSERT(game.textures->load("Projectile01.png", GL_LINEAR));

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
    background.texture = ASSERT_GET(game.textures->load("background03.png", GL_NEAREST));
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
    player.texture = ASSERT_GET(game.textures->load("Paranoid.png", GL_NEAREST));
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
    enemy.texture = ASSERT_GET(game.textures->load("UFO.png", GL_NEAREST));
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
    enemy.health = Health{ .value = 10 };
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
    const auto pixel_size = glm::vec2(1.f / game.viewport.size.x, 1.f / game.viewport.size.y);
    const auto cursor_pos = game.cursor.normalized(game.window, game.viewport);
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
          spaceship->health->value--;
          if (spaceship->health->value <= 0) {
            spaceship->delay_erasing = DelayErasing{};
            spaceship->transform = Transform{
              .position = glm::vec2(1000.0f),
            };
            spaceship->prev_transform = projectile.transform;
            game_pause(game);
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

/// Render ImGui windows
void imgui_render(Game& game)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  bool show_demo_window = true;
  ImGui::ShowDemoWindow(&show_demo_window);

  ImGui::Render();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow* backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }
}

/// Game render everything
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
  if (game.paused) {
    immediate_draw_text(generic_shader, "Qual das alternativas e uma Funcao Injetora?", std::nullopt, *game.fonts->russo_one, 50.f, kWhite, kBlack, 1.f);

    GameObject obj;
    obj.transform = Transform{
      .position = glm::vec2(0.f, -0.45f),
        .scale = glm::vec2(1.f, 0.3f),
        .rotation = 0.0f,
    };
    obj.texture = ASSERT_GET(game.textures->load("funcoes.png", GL_LINEAR));
    obj.glo = std::make_shared<GLObject>(create_textured_quad_globject(game.shaders->generic_shader));
    draw_textured_object(generic_shader, *obj.texture->get(), *obj.glo, obj.transform.matrix());
  }

  // Render Cursor
  //auto cursor_pos = normalized_cursor_pos(game.cursor, game.winsize);
  //auto cursor_obj = create_colored_quad_globject(generic_shader);
  //auto transform = Transform{};
  //transform.scale = glm::vec2(0.03f);
  //transform.position = glm::vec2(cursor_pos.x, cursor_pos.y);
  //draw_colored_object(generic_shader, cursor_obj, transform.matrix());

  // Render ImGui
  imgui_render(game);
}

int game_loop(GLFWwindow* window)
{
  //while (!glfwWindowShouldClose(window)) {
    //glfwPollEvents();
    //begin_render();
    //glfwSwapBuffers(window);
  //}
  //return 0;

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

  game->window.size.y = height;
  game->window.size.x = width;
  game->viewport.size.x = (width - x_rest);
  game->viewport.size.y = (height - y_rest);
  game->viewport.offset.x = x_off;
  game->viewport.offset.y = y_off;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (!game) return;
  game->cursor.pos.x = (float)xpos;
  game->cursor.pos.y = (float)ypos;
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

  // Init ImGui ================================================================
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF(ENGINE_ASSETS_PATH "/fonts/Ubuntu-Regular.ttf", 14.0f);
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

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
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();

  INFO("Exit");
  return ret;
}
