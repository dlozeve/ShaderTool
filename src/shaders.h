#ifndef SHADERS_H
#define SHADERS_H

#include "renderer.h"

int initialize_shaders(struct renderer_state *state, const char *shader_file,
                       const char *buffer_file, int window_width,
                       int window_height);
int compile_shaders(unsigned int *shader_program,
                    const char *const fragment_shader_file);
char *read_file(const char *const filename);

#endif /* SHADERS_H */
