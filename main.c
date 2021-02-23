#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>

#include "log.h"
#include "renderer.h"
#include "shaders.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

int main(int argc, char *argv[]) {
  if (argc < 2) {
    log_error("Usage: %s <fragment shader>", argv[0]);
    return EXIT_FAILURE;
  }
  const char *fragment_shader_file = argv[1];

  GLFWwindow *window = initialize_window(WINDOW_WIDTH, WINDOW_HEIGHT);
  if (window == NULL) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  unsigned int VAO = initialize_vertices();

  unsigned int shader_program = glCreateProgram();
  if (!shader_program) {
    log_error("Could not create shader program");
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }
  compile_shaders(&shader_program, fragment_shader_file);

  /* Drawing loop */
  size_t frame = 0;
  while (!glfwWindowShouldClose(window)) {
    process_input(window, &shader_program, fragment_shader_file);

    /* Background */
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);

    /* Setup uniforms */
    float time = glfwGetTime();
    int frag_time_location = glGetUniformLocation(shader_program, "u_time");
    glUniform1f(frag_time_location, time);

    int frag_frame_location = glGetUniformLocation(shader_program, "u_frame");
    glUniform1ui(frag_frame_location, frame);

    int viewport[4] = {0};
    glGetIntegerv(GL_VIEWPORT, viewport);
    int frag_resolution_location =
        glGetUniformLocation(shader_program, "u_resolution");
    glUniform2f(frag_resolution_location, viewport[2], viewport[3]);

    double mouse_x = 0, mouse_y = 0;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    int frag_mouse_resolution_location =
        glGetUniformLocation(shader_program, "u_mouse");
    glUniform2f(frag_mouse_resolution_location, mouse_x, mouse_y);

    /* Draw the vertices */
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); /* For wireframe mode */
    glBindVertexArray(VAO);
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
