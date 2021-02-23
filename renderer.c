#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "log.h"
#include "renderer.h"
#include "shaders.h"

#define UNUSED(a) (void)a

/**
 * @brief Initialize GLFW and OpenGL, and create a window.
 *
 * @param width The width of the window to create.
 * @param height The height of the window to create.
 * @return A pointer to the newly created GLFW window.
 */
GLFWwindow *initialize_window(int width, int height) {
  /* Initialize GLFW */
  if (!glfwInit()) {
    log_error("[GLFW] Failed to init");
    return NULL;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  /* Create window */
  GLFWwindow *window =
      glfwCreateWindow(width, height, "ShaderTool", NULL, NULL);
  if (window == NULL) {
    log_error("[GLFW] Failed to create window");
    glfwTerminate();
    return NULL;
  }
  glfwMakeContextCurrent(window);
  log_debug("[GLFW] Created window of size %d, %d", width, height);

  glViewport(0, 0, width, height);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  /* Initialize OpenGL */
  if (glewInit() != GLEW_OK) {
    log_error("[GLEW] Failed to initialize");
    return NULL;
  }
  log_debug("[GLEW] Initialized successfully");

  return window;
}

/**
 * @brief Initialize the vertex array.
 *
 * This functions defines a simple rectangle containing the whole
 * viewport.
 *
 * @return The vertex array object ID.
 */
unsigned int initialize_vertices() {
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

  return VAO;
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
 * @param fragment_shader_file The shader file to reload if needed.
 */
void process_input(GLFWwindow *window, unsigned int *shader_program,
                   const char *const fragment_shader_file) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    log_debug("Quitting");
    glfwSetWindowShouldClose(window, true);
  } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
    compile_shaders(shader_program, fragment_shader_file);
  }
}
