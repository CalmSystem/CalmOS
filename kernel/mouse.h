#ifndef MOUSE_H_
#define MOUSE_H_

#include "stdbool.h"

// Specific commands
#define MOUSE_SET_SAMPLE 0xF3
#define MOUSE_SET_RESOLUTION 0xE8
#define MOUSE_ENABLE_SCALING 0xE7
#define MOUSE_DISABLE_SCALING 0xE7

void init_mouse(int dev);
void mouse_set_sample_rate(unsigned char rate);
void mouse_set_scaling(bool enabled);
void mouse_set_resolution(unsigned char level);
void mouse_enable_scroll_wheel();
void mouse_enable_five_buttons();
void mouse_init();

#endif /*MOUSE_H_*/