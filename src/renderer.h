#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>

/**
 * Structure representing the state of a shader.
 */
struct shader_state {
  unsigned int program; /**< Shader program ID. */
  const char *filename; /**< Shader file name. */
  int wd;               /**< inotify watch descriptor. */
};

/**
 * Structure representing the state of the renderer and associated
 * shaders.
 */
struct renderer_state {
  GLFWwindow *window; /**< GLFW window where the shaders are rendered. */
  struct shader_state screen_shader; /**< Shader for the main screen. */
  struct shader_state buffer_shader; /**< Shader for the framebuffer. */
  unsigned int framebuffer;          /**< Framebuffer. */
  unsigned int
      texture_color_buffer; /**< Texture where the framebuffer renders. */
  int inotify_fd;           /**< inotify file descriptor. */
  size_t frame_count; /**< Frame count since the start of the render loop. */
  size_t prev_frame_count; /**< Frame count at the last log. */
  double time;      /**< Time in seconds since the start of the render loop. */
  double prev_time; /**< Time in seconds at the last log. */
};

GLFWwindow *initialize_window(int width, int height);
unsigned int initialize_vertices();
unsigned int initialize_framebuffer(unsigned int *framebuffer,
                                    unsigned int *texture_color_buffer,
                                    unsigned int texture_width,
                                    unsigned int texture_height);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

#endif /* RENDERER_H */
