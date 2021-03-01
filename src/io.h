#ifndef IO_H
#define IO_H

#include "renderer.h"

char *basename_without_suffix(const char *filename);
void capture_screenshot(struct renderer_state *state);
void process_input(struct renderer_state *state);

#endif /* IO_H */
