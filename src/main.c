#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <argp.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/inotify.h>

#include "io.h"
#include "log.h"
#include "renderer.h"
#include "shaders.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

const char *argp_program_version = "0.1";
const char *argp_program_bug_address =
    "https://github.com/dlozeve/ShaderTool/issues";
static char doc[] =
    "ShaderTool -- Live tool for developing OpenGL shaders interactively";
static char args_doc[] = "SHADER\v"
                         "Compile and render the SHADER.";

static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Produce verbose output", 0},
    {"silent", 's', 0, 0, "Don't produce any output", 0},
    {"quiet", 'q', 0, OPTION_ALIAS, 0, 0},
    {"buffer", 'b', "FILE", 0, "Source file of the buffer fragment shader", 0},
    {0},
};

struct arguments {
  char *shader_file;
  int verbose;
  int silent;
  char *buffer_file;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
  case 'v':
    arguments->verbose = true;
    break;
  case 's':
  case 'q':
    arguments->silent = true;
    break;
  case 'b':
    arguments->buffer_file = arg;
    break;

  case ARGP_KEY_ARG:
    if (state->arg_num >= 1) {
      /* Too many arguments */
      argp_usage(state);
    }
    arguments->shader_file = arg;
    break;

  case ARGP_KEY_END:
    if (state->arg_num < 1) {
      /* Not enough arguments */
      argp_usage(state);
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp_parser = {
    .options = options, .parser = parse_opt, .args_doc = args_doc, .doc = doc};

int main(int argc, char *argv[]) {
  struct arguments arguments = {0};
  /* Default values */
  arguments.verbose = false;
  arguments.silent = false;
  arguments.buffer_file = 0;

  argp_parse(&argp_parser, argc, argv, 0, 0, &arguments);

  if (arguments.silent) {
    log_set_level(LOG_ERROR);
  } else if (arguments.verbose) {
    log_set_level(LOG_DEBUG);
  } else {
    log_set_level(LOG_INFO);
  }

  struct renderer_state state = {0};

  /* Create inotify instance */
  state.inotify_fd = inotify_init();
  if (state.inotify_fd == -1) {
    log_warn("[inotify] Cannot initialize inotify");
    perror("inotify_init");
  } else {
    /* Set the inotify file descriptor to be non-blocking */
    if (!fcntl(state.inotify_fd, F_SETFL, O_NONBLOCK)) {
      log_debug("[inotify] Initialized successfully");
    }
  }

  state.screen_shader.filename = arguments.shader_file;
  log_info("Screen shader file: %s", state.screen_shader.filename);

  if (state.inotify_fd != -1) {
    state.screen_shader.wd = inotify_add_watch(
        state.inotify_fd, state.screen_shader.filename, IN_MODIFY);
    if (state.screen_shader.wd == -1) {
      log_warn("[inotify] Cannot watch file %s", state.screen_shader.filename);
      perror("inotify_add_watch");
    } else {
      log_debug("[inotify] Watching file %s", state.screen_shader.filename);
    }
  }

  if (arguments.buffer_file) {
    state.buffer_shader.filename = arguments.buffer_file;
    log_info("Buffer shader file: %s", state.buffer_shader.filename);

    if (state.inotify_fd != -1) {
      state.buffer_shader.wd = inotify_add_watch(
          state.inotify_fd, state.buffer_shader.filename, IN_MODIFY);
      if (state.buffer_shader.wd == -1) {
        log_warn("[inotify] Cannot watch file %s",
                 state.buffer_shader.filename);
        perror("inotify_add_watch");
      } else {
        log_debug("[inotify] Watching file %s", state.buffer_shader.filename);
      }
    }
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
      log_info("frame = %zu, time = %.2f, fps = %.2f, viewport = (%d, %d)",
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
