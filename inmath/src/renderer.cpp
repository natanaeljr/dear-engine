#include "./renderer.hpp"

#include <glbinding/gl33core/gl.h>
using namespace gl;
#include <glm/gtc/type_ptr.hpp>

#include "./api/sprite.hpp"
#include "./api/gl_object.hpp"
#include "./api/gl_shader.hpp"
#include "./api/gl_texture.hpp"

/// Prepare to render
void begin_render()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
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
                          const glm::mat4& model, const SpriteFrame* sprite)
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

