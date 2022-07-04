#include "./camera.hpp"

#include <glbinding/gl33core/gl.h>
using namespace gl;
#include <glm/gtc/type_ptr.hpp>

#include "gl_shader.hpp"

/// Upload camera matrix to shader
void set_camera(const GLShader& shader, const Camera& camera)
{
  glUniformMatrix4fv(shader.unif_loc(GLUnif::VIEW), 1, GL_FALSE, glm::value_ptr(camera.view));
  glUniformMatrix4fv(shader.unif_loc(GLUnif::PROJECTION), 1, GL_FALSE, glm::value_ptr(camera.projection));
}

