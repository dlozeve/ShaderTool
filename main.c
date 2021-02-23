#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "main.h"

#define UNUSED(a) (void)a

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define FRAGMENT_SHADER_FILE "main.frag"

int main() {
  /* Initialize GLFW */
  if (!glfwInit()) {
    log_error("[GLFW] Failed to init");
    return EXIT_FAILURE;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  /* Create window */
  GLFWwindow *window =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Test", NULL, NULL);
  if (window == NULL) {
    log_error("[GLFW] Failed to create window");
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  log_debug("[GLFW] Created window of size %d, %d", WINDOW_WIDTH,
            WINDOW_HEIGHT);

  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  /* Initialize OpenGL */
  if (glewInit() != GLEW_OK) {
    log_error("[GLEW] Failed to initialize");
    return EXIT_FAILURE;
  }
  log_debug("[GLEW] Initialized successfully");

  /* Define vertices */
  float vertices[] = {
      -1.0, -1.0, 0.0, // bottom left
      -1.0, 1.0,  0.0, // top left
      1.0,  -1.0, 0.0, // top right
      1.0,  1.0,  0.0  // bottom right
  };
  unsigned int indices[] = {
      0, 1, 2, // first triangle
      1, 2, 3  // second triangle
  };

  unsigned int VBO = 0;
  glGenBuffers(1, &VBO);
  unsigned int EBO = 0;
  glGenBuffers(1, &EBO);
  unsigned int VAO = 0;
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  log_debug("Vertex data initialized successfully");

  unsigned int shader_program = glCreateProgram();
  int result = compile_shaders(&shader_program, FRAGMENT_SHADER_FILE);
  if (!shader_program || result) {
    log_error("Could not compile shaders");
    return EXIT_FAILURE;
  }

  /* Drawing loop */
  size_t frame = 0;
  while (!glfwWindowShouldClose(window)) {
    process_input(window, &shader_program);

    /* Background */
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);

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

/**
 * @brief Compile shaders from source files.
 *
 * This function reads the source files of the vertex and fragment
 * shaders, compiles them, and links them together in a shader
 * program.
 *
 * @param shader_program Pointer to the ID of the shader program
 * where the shaders will be stored.
 * @param fragment_shader_file File name of the fragment shader.
 * @return 0 on success, 1 on error.
 */
int compile_shaders(unsigned int *shader_program,
                    const char *const fragment_shader_file) {
  /* Compile vertex shader */
  const char *const vertex_shader_source =
      "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "void main()\n"
      "{\n"
      "  gl_Position = vec4(aPos, 1.0);\n"
      "}\n";

  unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  int success = 0;
  char info_log[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
    log_error("Vertex shader compilation failed: %s", info_log);
  }

  /* Compile fragment shader */
  const char *const fragment_shader_source = read_file(fragment_shader_file);
  if (fragment_shader_source == NULL) {
    log_error("Could not load fragment shader from file %s",
              fragment_shader_file);
    return 1;
  }
  unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
    log_error("Fragment shader compilation failed: %s", info_log);
    return 1;
  }

  /* Link shaders */
  unsigned int new_shader_program = glCreateProgram();
  glAttachShader(new_shader_program, vertex_shader);
  glAttachShader(new_shader_program, fragment_shader);
  glLinkProgram(new_shader_program);
  glGetShaderiv(new_shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(new_shader_program, 512, NULL, info_log);
    log_error("Shader program linking failed: %s", info_log);
    return 1;
  }

  glDeleteProgram(*shader_program);
  *shader_program = new_shader_program;

  free((void *)fragment_shader_source);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  log_debug("Shaders compiled successfully");

  return 0;
}

/**
 * @brief Callback to adjust the size of the viewport when the window
 * is resized.
 *
 * @param window The current window.
 * @param width The new width.
 * @param height The new height.
 */
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  UNUSED(window);
  glViewport(0, 0, width, height);
}

/**
 * @brief Ensure the window is closed when the user presses the escape
 * key.
 *
 * @param window The current window.
 * @param shader_program Pointer to the shader program to update if needed.
 */
void process_input(GLFWwindow *window, unsigned int *shader_program) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    log_debug("Quitting");
    glfwSetWindowShouldClose(window, true);
  } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
    compile_shaders(shader_program, FRAGMENT_SHADER_FILE);
  }
}

/**
 * @brief Reads a file in a heap-allocated buffer.
 *
 * This function computes the length of the file, allocate a buffer of
 * the correct size, and reads the file in the buffer. Returns `NULL`
 * on error.
 *
 * @param filename The file to read.
 * @return A buffer containing the contents of the file, or `NULL` if
 * there was an error.
 */
char *read_file(const char *const filename) {
  FILE *fd = fopen(filename, "r");
  if (fd == NULL) {
    log_error("Could not open file %s", filename);
    return NULL;
  }

  if (fseek(fd, 0, SEEK_END) == -1) {
    perror("fseek");
    return NULL;
  }
  long size = ftell(fd);
  if (size == -1) {
    perror("ftell");
    return NULL;
  }
  rewind(fd);
  char *buf = calloc(size + 1, 1);
  if (buf == NULL) {
    log_error("Failed to allocate memory to read file %s", filename);
    return NULL;
  }

  long bytes_read = fread(buf, 1, size, fd);
  if (bytes_read != size) {
    log_error("Failed to read file %s (%ld bytes read out of %ld total)",
              filename, bytes_read, size);
    return NULL;
  }

  fclose(fd);
  return buf;
}
