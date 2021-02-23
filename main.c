#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>

#include "log.h"
#include "shaders.h"
#include "renderer.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define FRAGMENT_SHADER_FILE "main.frag"

int main() {
  GLFWwindow *window = initialize_window(WINDOW_WIDTH, WINDOW_HEIGHT);

  unsigned int VAO = initialize_vertices();

  unsigned int shader_program = glCreateProgram();
  int result = compile_shaders(&shader_program, FRAGMENT_SHADER_FILE);
  if (!shader_program || result) {
    log_error("Could not compile shaders");
    return EXIT_FAILURE;
  }

  /* Drawing loop */
  size_t frame = 0;
  while (!glfwWindowShouldClose(window)) {
    process_input(window, &shader_program, FRAGMENT_SHADER_FILE);

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

  glfwTerminate();
  return EXIT_SUCCESS;
}

