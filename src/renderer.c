#include <FreeImage.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "renderer.h"
#include "shaders.h"

#define UNUSED(a) (void)a
#define BUF_LEN (10 * (sizeof(struct inotify_event) + 1))

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

/**
 * @brief Return the file name without the leading directories and
 * without the extension.
 *
 * @param filename The original filename.
 * @return A newly allocated string containing the output of basename(3)
 * without the extension.
 */
char *basename_without_suffix(const char *filename) {
  char *name = strdup(filename);
  name = basename(name);
  log_debug("%s", name);
  char *dot = strrchr(name, '.');
  if (!dot || dot == name) {
    return name;
  }
  *dot = '\0';
  log_debug("%s", name);
  return name;
}

/**
 * @brief Capture a screenshot of the current window.
 *
 * Takes the dimensions of the viewport to save a pixel array of the
 * same dimensions. Uses FreeImage to create an image from the raw
 * pixels and save it to disk.
 *
 * @param state The renderer state, needed to get the name of the
 * current shader and the frame count.
 */
void capture_screenshot(struct renderer_state *state) {
  time_t now = time(NULL);
  struct tm *timenow = gmtime(&now);
  char image_filename[255] = {0};
  char *shader_basename =
      basename_without_suffix(state->screen_shader.filename);
  snprintf(image_filename, sizeof(image_filename),
           "%s_%zu_%d%02d%02d_%02d%02d%02d.png", shader_basename,
           state->frame_count, timenow->tm_year + 1900, timenow->tm_mon,
           timenow->tm_mday, timenow->tm_hour, timenow->tm_min,
           timenow->tm_sec);

  int viewport[4] = {0};
  glGetIntegerv(GL_VIEWPORT, viewport);

  GLubyte *pixels = calloc(3 * viewport[2] * viewport[3], 1);
  glReadPixels(0, 0, viewport[2], viewport[3], GL_BGR, GL_UNSIGNED_BYTE,
               pixels);

  FIBITMAP *image = FreeImage_ConvertFromRawBits(
      pixels, viewport[2], viewport[3], 3 * viewport[2], 24, FI_RGBA_RED_MASK,
      FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false);

  if (FreeImage_Save(FIF_PNG, image, image_filename, 0)) {
    log_debug("Image saved to %s", image_filename);
  } else {
    log_error("Failed to saved image to %s", image_filename);
  }

  FreeImage_Unload(image);
  free(pixels);
}

/**
 * @brief Ensure the window is closed when the user presses the escape
 * key.
 *
 * @param state The current state of the renderer.
 */
void process_input(struct renderer_state *state) {
  bool should_reload = false;

  // Skip inotify checking if it's not available
  if (state->inotify_fd != -1 &&
      (state->screen_shader.wd != -1 || state->buffer_shader.wd != -1)) {
    char buf[BUF_LEN] = {0};
    int num_read = read(state->inotify_fd, buf, BUF_LEN);
    if (num_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      // No event, do nothing
    } else if (num_read <= 0) {
      log_error("[inotify] Could not read inotify state");
    } else {
      should_reload = true;
    }
  }

  if (glfwGetKey(state->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    log_info("Quitting");
    glfwSetWindowShouldClose(state->window, true);
  } else if (should_reload ||
             glfwGetKey(state->window, GLFW_KEY_R) == GLFW_PRESS) {
    if (should_reload) {
      log_info("File changed on disk, reloading shaders");
    } else {
      log_info("Reloading shaders");
    }
    // reinitialize time and frame count
    state->frame_count = 0;
    state->prev_frame_count = 0;
    glfwSetTime(0.0);
    state->time = 0.0;
    state->prev_time = 0.0;
    // recompile shaders
    compile_shaders(&state->screen_shader.program,
                    state->screen_shader.filename);
    if (state->buffer_shader.filename) {
      compile_shaders(&state->buffer_shader.program,
                      state->buffer_shader.filename);
    }
  } else if (glfwGetKey(state->window, GLFW_KEY_S) == GLFW_PRESS) {
    capture_screenshot(state);
  }
}
