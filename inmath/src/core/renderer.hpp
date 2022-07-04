#pragma once

#include <glm/mat4x4.hpp>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Renderer

/// Prepare to render
void begin_render();

/// Render a colored GLObject with indices
void draw_colored_object(const class GLShader& shader, const struct GLObject& glo, const glm::mat4& model);


/// Render a textured GLObject with indices
void draw_textured_object(const class GLShader& shader, const struct GLTexture& texture, const struct GLObject& glo,
                          const glm::mat4& model, const struct SpriteFrame* sprite = nullptr);


/// Render a text GLObject with indices
void draw_text_object(const class GLShader& shader, const struct GLTexture& texture, const struct GLObject& glo,
                      const glm::mat4& model, const glm::vec4& color, const glm::vec4& outline_color, const float outline_thickness);

