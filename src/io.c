#include <FreeImage.h>
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

#define BUF_LEN (10 * (sizeof(struct inotify_event) + 1))

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
