#ifndef CONSOLE_H_
#define CONSOLE_H_
#include "stdint.h"

#define REDCROSS_COL (CONSOLE_COL - 1)
#define REDCROSS_LIG 0

void console_putbytes(const char *chaine, int32_t taille);
/** Writes at specific col and lig. Cursor rollback to previous position */
void console_putbytes_at(const char *chaine, int32_t taille, uint8_t col, uint8_t lig);
/** Defines current foreground color */
void console_set_foreground(uint8_t c);
/** Defines current background color */
void console_set_background(uint8_t c);

/** Set the background color to c at a specific col and lig */
void console_set_background_at(uint8_t col, uint8_t lig, uint8_t c);
/** Writes a red cross at REDCROSS_COL and REDCROSS_LIG */
void console_draw_red_cross();

#endif /*CONSOLE_H_*/
