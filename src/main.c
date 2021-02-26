#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <sys/inotify.h>

#include "log.h"
#include "renderer.h"
#include "shaders.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

int main(int argc, char *argv[]) {
  if (argc < 2) {
    log_error("Usage: %s <screen shader> [<buffer shader>]", argv[0]);
    return EXIT_FAILURE;
  }

  struct renderer_state state = {0};

  /* Create inotify instance */
  state.inotify_fd = inotify_init();
  if (state.inotify_fd == -1) {
    log_error("Cannot initialize inotify");
    perror("inotify_init");
    return EXIT_FAILURE;
  }

  state.screen_shader.filename = argv[1];
  state.screen_shader.wd =
      inotify_add_watch(state.inotify_fd, state.screen_shader.filename, IN_MODIFY);
  if (state.screen_shader.wd == -1) {
    log_error("Cannot watch file %s", state.screen_shader.filename);
    perror("inotify_add_watch");
    return EXIT_FAILURE;
  }
  log_debug("Screen shader file: %s", state.screen_shader.filename);
  if (argc >= 3) {
    state.buffer_shader.filename = argv[2];
    state.buffer_shader.wd =
        inotify_add_watch(state.inotify_fd, state.buffer_shader.filename, IN_MODIFY);
    if (state.buffer_shader.wd == -1) {
      log_error("Cannot watch file %s", state.buffer_shader.filename);
      perror("inotify_add_watch");
      return EXIT_FAILURE;
    }
    log_debug("Buffer shader file: %s", state.buffer_shader.filename);
  }

  state.window = initialize_window(WINDOW_WIDTH, WINDOW_HEIGHT);
  if (state.window == NULL) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  unsigned int VAO = initialize_vertices();

  state.screen_shader.program = glCreateProgram();
  if (!state.screen_shader.program) {
    log_error("Could not create screen shader program");
    glfwDestroyWindow(state.window);
    glfwTerminate();
    return EXIT_FAILURE;
  }
  compile_shaders(&state.screen_shader.program, state.screen_shader.filename);
  glUseProgram(state.screen_shader.program);
  glUniform1i(glGetUniformLocation(state.screen_shader.program, "u_texture"),
              0);

  unsigned int framebuffer = 0;
  unsigned int texture_color_buffer = 0;
  if (state.buffer_shader.filename) {
    state.buffer_shader.program = glCreateProgram();
    if (!state.buffer_shader.program) {
      log_error("Could not create buffer shader program");
      glfwDestroyWindow(state.window);
      glfwTerminate();
      return EXIT_FAILURE;
    }
    compile_shaders(&state.buffer_shader.program, state.buffer_shader.filename);
    glUseProgram(state.buffer_shader.program);
    glUniform1i(glGetUniformLocation(state.buffer_shader.program, "u_texture"),
                0);

    if (initialize_framebuffer(&framebuffer, &texture_color_buffer,
                               WINDOW_WIDTH, WINDOW_HEIGHT)) {
      glfwDestroyWindow(state.window);
      glfwTerminate();
      return EXIT_FAILURE;
    }
  }

  /* Drawing loop */
  glfwSetTime(0.0);
  while (!glfwWindowShouldClose(state.window)) {
    process_input(&state);

    /* data required for uniforms */
    state.time = glfwGetTime();
    int viewport[4] = {0};
    glGetIntegerv(GL_VIEWPORT, viewport);
    double mouse_x = 0, mouse_y = 0;
    glfwGetCursorPos(state.window, &mouse_x, &mouse_y);

    if (state.time - state.prev_time >= 1.0) {
      double fps = (state.frame_count - state.prev_frame_count) /
                   (state.time - state.prev_time);
      log_debug("frame = %zu, time = %.2f, fps = %.2f, viewport = (%d, %d)",
                state.frame_count, state.time, fps, viewport[2], viewport[3]);
      state.prev_frame_count = state.frame_count;
      state.prev_time = state.time;
    }

    if (state.buffer_shader.filename) {
      /* bind the framebuffer and draw to it */
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

      /* Background */
      glClearColor(0, 0, 0, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      /* Setup uniforms */
      glUseProgram(state.buffer_shader.program);
      glUniform1ui(glGetUniformLocation(state.buffer_shader.program, "u_frame"),
                   state.frame_count);
      glUniform1f(glGetUniformLocation(state.buffer_shader.program, "u_time"),
                  state.time);
      glUniform2f(
          glGetUniformLocation(state.buffer_shader.program, "u_resolution"),
          viewport[2], viewport[3]);
      glUniform2f(glGetUniformLocation(state.buffer_shader.program, "u_mouse"),
                  mouse_x, mouse_y);

      /* Draw the vertices */
      glBindVertexArray(VAO);
      glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
    }

    /* bind back to default framebuffer */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Setup uniforms */
    glUseProgram(state.screen_shader.program);
    glUniform1ui(glGetUniformLocation(state.screen_shader.program, "u_frame"),
                 state.frame_count);
    glUniform1f(glGetUniformLocation(state.screen_shader.program, "u_time"),
                state.time);
    glUniform2f(
        glGetUniformLocation(state.screen_shader.program, "u_resolution"),
        viewport[2], viewport[3]);
    glUniform2f(glGetUniformLocation(state.screen_shader.program, "u_mouse"),
                mouse_x, mouse_y);

    /* Draw the vertices */
    glBindVertexArray(VAO);
    glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glfwSwapBuffers(state.window);
    glfwPollEvents();
    state.frame_count++;
  }

  glfwDestroyWindow(state.window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
