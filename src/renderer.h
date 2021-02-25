#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>

/**
 * Structure representing the state of the renderer and associated
 * shaders.
 */
struct renderer_state {
  GLFWwindow *window;             /**< GLFW window where the shaders are rendered. */
  unsigned int screen_shader;     /**< Shader for the main screen. */
  const char *screen_shader_file; /**< Screen shader file name. */
  unsigned int buffer_shader;     /**< Shader for the background framebuffer. */
  const char *buffer_shader_file; /**< Buffer shader file name. */
  size_t frame_count;		  /**< Frame count since the beginning of the render loop. */
  size_t prev_frame_count;	  /**< Frame count at the last log. */
  double time;			  /**< Time in seconds since the beginning of the render loop. */
  double prev_time;		  /**< Time in seconds at the last log. */
};

GLFWwindow *initialize_window(int width, int height);
unsigned int initialize_vertices();
unsigned int initialize_framebuffer(unsigned int *framebuffer,
                                    unsigned int *texture_color_buffer,
                                    unsigned int texture_width,
                                    unsigned int texture_height);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void capture_screenshot();
void process_input(struct renderer_state *state);

#endif /* RENDERER_H */
