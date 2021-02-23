#ifndef SHADERS_H
#define SHADERS_H

char *read_file(const char *const filename);
int compile_shaders(unsigned int *shader_program,
                    const char *const fragment_shader_file);

#endif /* SHADERS_H */
