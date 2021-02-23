#ifndef MAIN_H
#define MAIN_H

#include <GLFW/glfw3.h>

char *read_file(const char *const filename);
int compile_shaders(unsigned int *shader_program,
                    const char *const fragment_shader_file);
void process_input(GLFWwindow *window, unsigned int *shader_program);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

#endif /* MAIN_H */
