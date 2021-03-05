#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>

#include "log.h"
#include "renderer.h"
#include "shaders.h"

/**
 * @brief Initialize shaders, compile them, and create the required
 * texture for the buffer shader.
 *
 * @param state The target renderer state.
 * @param shader_file The file name of the screen shader.
 * @param buffer_file The file name of the buffer shader, or NULL if no buffer
 * shader.
 * @param texture_width The width of the texture for the buffer shader.
 * @param texture_height The height of the texture for the buffer shader.
 * @return 0 on success, 1 on error.
 */
int initialize_shaders(struct renderer_state *state, const char *shader_file,
                       const char *buffer_file, int texture_width,
                       int texture_height) {
  state->screen_shader.filename = shader_file;
  log_info("Screen shader file: %s", state->screen_shader.filename);

  if (state->inotify_fd != -1) {
    state->screen_shader.wd = inotify_add_watch(
        state->inotify_fd, state->screen_shader.filename, IN_MODIFY);
    if (state->screen_shader.wd == -1) {
      log_warn("[inotify] Cannot watch file %s", state->screen_shader.filename);
      perror("inotify_add_watch");
    } else {
      log_debug("[inotify] Watching file %s", state->screen_shader.filename);
    }
  }

  if (buffer_file) {
    state->buffer_shader.filename = buffer_file;
    log_info("Buffer shader file: %s", state->buffer_shader.filename);

    if (state->inotify_fd != -1) {
      state->buffer_shader.wd = inotify_add_watch(
          state->inotify_fd, state->buffer_shader.filename, IN_MODIFY);
      if (state->buffer_shader.wd == -1) {
        log_warn("[inotify] Cannot watch file %s",
                 state->buffer_shader.filename);
        perror("inotify_add_watch");
      } else {
        log_debug("[inotify] Watching file %s", state->buffer_shader.filename);
      }
    }
  }

  state->screen_shader.program = glCreateProgram();
  if (!state->screen_shader.program) {
    log_error("Could not create screen shader program");
    return 1;
  }
  compile_shaders(&state->screen_shader.program, state->screen_shader.filename);
  glUseProgram(state->screen_shader.program);
  glUniform1i(glGetUniformLocation(state->screen_shader.program, "u_texture"),
              0);

  if (state->buffer_shader.filename) {
    state->buffer_shader.program = glCreateProgram();
    if (!state->buffer_shader.program) {
      log_error("Could not create buffer shader program");
      return 1;
    }
    compile_shaders(&state->buffer_shader.program,
                    state->buffer_shader.filename);
    glUseProgram(state->buffer_shader.program);
    glUniform1i(glGetUniformLocation(state->buffer_shader.program, "u_texture"),
                0);

    if (initialize_framebuffer(&state->framebuffer,
                               &state->texture_color_buffer, texture_width,
                               texture_height)) {
      return 1;
    }
  }

  return 0;
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
  log_debug("Compiling %s", fragment_shader_file);
  /* Compile vertex shader */
  const char *const vertex_shader_source =
      "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "layout (location = 1) in vec2 aTexCoord;\n"
      "out vec2 TexCoord;\n"
      "void main()\n"
      "{\n"
      "  gl_Position = vec4(aPos, 1.0);\n"
      "  TexCoord = aTexCoord;\n"
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
