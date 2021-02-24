#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>

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
  const char *screen_shader_file = argv[1];
  log_debug("Screen shader file: %s", screen_shader_file);
  const char *buffer_shader_file = NULL;
  if (argc >= 3) {
    buffer_shader_file = argv[2];
    log_debug("Buffer shader file: %s", buffer_shader_file);
  }

  GLFWwindow *window = initialize_window(WINDOW_WIDTH, WINDOW_HEIGHT);
  if (window == NULL) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  unsigned int VAO = initialize_vertices();

  unsigned int screen_shader = glCreateProgram();
  if (!screen_shader) {
    log_error("Could not create screen shader program");
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }
  compile_shaders(&screen_shader, screen_shader_file);
  glUseProgram(screen_shader);
  glUniform1i(glGetUniformLocation(screen_shader, "u_texture"), 0);

  unsigned int buffer_shader = 0;
  unsigned int framebuffer = 0;
  unsigned int texture_color_buffer = 0;
  if (buffer_shader_file) {
    buffer_shader = glCreateProgram();
    if (!buffer_shader) {
      log_error("Could not create buffer shader program");
      glfwDestroyWindow(window);
      glfwTerminate();
      return EXIT_FAILURE;
    }
    compile_shaders(&buffer_shader, buffer_shader_file);
    glUseProgram(buffer_shader);
    glUniform1i(glGetUniformLocation(buffer_shader, "u_texture"), 0);

    /* Framebuffer */
    framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    /* color attachment texture */
    glGenTextures(1, &texture_color_buffer);
    glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture_color_buffer, 0);
    /* check that the framebuffer is complete */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      log_error("Framebuffer is not complete");
      glfwDestroyWindow(window);
      glfwTerminate();
      return EXIT_FAILURE;
    } else {
      log_debug("Framebuffer initialized and complete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  /* Drawing loop */
  size_t frame = 0;
  while (!glfwWindowShouldClose(window)) {
    process_input(window, &screen_shader, screen_shader_file, &buffer_shader,
                  buffer_shader_file);

    /* data required for uniforms */
    float time = glfwGetTime();
    int viewport[4] = {0};
    glGetIntegerv(GL_VIEWPORT, viewport);
    double mouse_x = 0, mouse_y = 0;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);

    if (frame % 100 == 0) {
      log_debug("frame = %zu, time = %f, viewport = (%d, %d)", frame, time,
                viewport[2], viewport[3]);
    }

    if (buffer_shader_file) {
      /* bind the framebuffer and draw to it */
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

      /* Background */
      glClearColor(0, 0, 0, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      /* Setup uniforms */
      glUniform1ui(glGetUniformLocation(buffer_shader, "u_frame"), frame);
      glUniform1f(glGetUniformLocation(buffer_shader, "u_time"), time);
      glUniform2f(glGetUniformLocation(buffer_shader, "u_resolution"),
                  viewport[2], viewport[3]);
      glUniform2f(glGetUniformLocation(buffer_shader, "u_mouse"), mouse_x,
                  mouse_y);

      /* Draw the vertices */
      glBindVertexArray(VAO);
      glUseProgram(buffer_shader);
      glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);

      /* bind back to default framebuffer */
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Setup uniforms */
    glUniform1ui(glGetUniformLocation(screen_shader, "u_frame"), frame);
    glUniform1f(glGetUniformLocation(screen_shader, "u_time"), time);
    glUniform2f(glGetUniformLocation(screen_shader, "u_resolution"),
                viewport[2], viewport[3]);
    glUniform2f(glGetUniformLocation(screen_shader, "u_mouse"), mouse_x,
                mouse_y);

    /* Draw the vertices */
    glBindVertexArray(VAO);
    glUseProgram(screen_shader);
    glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();
    frame++;
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
