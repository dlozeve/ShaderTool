#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>

GLFWwindow *initialize_window(int width, int height);
unsigned int initialize_vertices();
void process_input(GLFWwindow *window, unsigned int *shader_program,
                   const char *const fragment_shader_file);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

#endif /* RENDERER_H */
