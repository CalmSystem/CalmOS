#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "console.h"

#define BUFFER_SIZE CONSOLE_COL*CONSOLE_LIG

int get_buffer();

void init_keyboard_buffer();

void keyboard_data(char *str);
// void kbd_leds(unsigned char leds);

void cons_echo(int on);
unsigned long cons_read(char *string, unsigned long length);
int cons_write(const char *str, long size);

#endif /*KEYBOARD_H_*/