#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "log.h"
#include "renderer.h"

#define UNUSED(a) (void)a

/**
 * @brief Initialize GLFW and OpenGL, and create a window.
 *
 * @param width The width of the window to create.
 * @param height The height of the window to create.
 * @return A pointer to the newly created GLFW window, or `NULL` on error.
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
      // positions     // texture coords
      1.0,  1.0,  0.0, 1.0, 1.0, // top right
      1.0,  -1.0, 0.0, 1.0, 0.0, // bottom right
      -1.0, -1.0, 0.0, 0.0, 0.0, // bottom left
      -1.0, 1.0,  0.0, 0.0, 1.0, // top left
  };
  unsigned int indices[] = {
      0, 1, 3, // first triangle
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

  /* position attribute */
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  /* texture coord attribute */
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  log_debug("Vertex data initialized successfully");

  return VAO;
}

/**
 * @brief Initialize a framebuffer and the associated texture.
 *
 * @param framebuffer The framebuffer ID to be initialized.
 * @param texture_color_buffer The texture ID to be initialized.
 * @param texture_width The width of the desired texture image.
 * @param texture_height The height of the desired texture image.
 * @return 0 on success, 1 on failure.
 */
unsigned int initialize_framebuffer(unsigned int *framebuffer,
                                    unsigned int *texture_color_buffer,
                                    unsigned int texture_width,
                                    unsigned int texture_height) {
  glGenFramebuffers(1, framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
  /* color attachment texture */
  glGenTextures(1, texture_color_buffer);
  glBindTexture(GL_TEXTURE_2D, *texture_color_buffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0,
               GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         *texture_color_buffer, 0);
  /* check that the framebuffer is complete */
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    log_error("Framebuffer is not complete");
    return 1;
  }
  log_debug("Framebuffer initialized and complete");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
