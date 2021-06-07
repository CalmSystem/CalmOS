#ifndef CONSOLE_H_
#define CONSOLE_H_
#include "stdint.h"

void console_putbytes(char *chaine, int32_t taille);
void console_set_foreground(uint8_t c);
void console_set_background(uint8_t c);

#define CONSOLE_BLACK 0
#define CONSOLE_BLUE 1
#define CONSOLE_GREEN 2
#define CONSOLE_CYAN 3
#define CONSOLE_RED 4
#define CONSOLE_MAGENTA 5
#define CONSOLE_BROWN 6
#define CONSOLE_LIGHT_GREY 7
#define CONSOLE_GREY 8
#define CONSOLE_LIGHT_BLUE 9
#define CONSOLE_LIGHT_GREEN 10
#define CONSOLE_LIGHT_CYAN 11
#define CONSOLE_LIGHT_RED 12
#define CONSOLE_LIGHT_MAGENTA 13
#define CONSOLE_YELLOW 14
#define CONSOLE_WHITE 15

#endif /*CONSOLE_H_*/
