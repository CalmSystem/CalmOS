#include "cpu.h"
#include "stdint.h"
#include "console.h"
#include "string.h"

uint16_t* const DISPLAY_START = (uint16_t*)0xB8000;
const uint16_t CURSOR_PORT_CMD = 0x3D4;
const uint16_t CURSOR_PORT_DATA = 0x3D5;
const uint8_t NB_COL = 80;
const uint8_t NB_LIG = 25;

struct state_t
{
  uint8_t col, lig;
  uint8_t ct, cf;
};
struct state_t state = {0, 0, CONSOLE_WHITE, CONSOLE_BLACK};

uint16_t* char_addr(uint8_t col, uint8_t lig)
{
  return DISPLAY_START + (lig * NB_COL + col);
}

uint16_t pack_char(uint8_t text, uint8_t ct, uint8_t cf)
{
  const uint8_t CL = 0;
  uint16_t color = (CL << 7) | (cf << 4) | (ct);
  return (color << 8) | (text);
}
void put_cursor(uint8_t col, uint8_t lig)
{
  const uint16_t at = col + lig * NB_COL;
  outb(0x0F, CURSOR_PORT_CMD);
  outb(at, CURSOR_PORT_DATA);
  outb(0x0E, CURSOR_PORT_CMD);
  outb(at >> 8, CURSOR_PORT_DATA);
}

void screen_clean()
{
  const uint16_t clean = pack_char(' ', CONSOLE_WHITE, CONSOLE_BLACK);
  for (uint16_t i = 0; i < NB_LIG * NB_COL; i++) {
    *(DISPLAY_START + i) = clean;
  }
}
void screen_scroll()
{
  const uint16_t bottom = (NB_LIG - 1) * NB_COL;
  memmove(DISPLAY_START, DISPLAY_START + NB_COL, sizeof(uint16_t) * bottom);

  const uint16_t clean = pack_char(' ', CONSOLE_WHITE, CONSOLE_BLACK);
  for (uint16_t i = 0; i < NB_COL; i++) {
    *(DISPLAY_START + bottom + i) = clean;
  }
}

void wrap_cursor()
{
  if (state.col >= NB_COL) {
    state.col = 0;
    state.lig++;
  }
  if (state.lig >= NB_LIG) {
    screen_scroll();
    state.lig = NB_LIG - 1;
  }
}
void handle_char(char c)
{
  switch (c)
  {
  case 8:
    if (state.col > 0) state.col--;
    break;

  case 9:
    state.lig = ((state.lig / 8) + 1) * 8;
    if (state.lig >= NB_LIG) state.lig = 79;
    break;

  case 10:
    state.col = 0;
    state.lig++;
    wrap_cursor();
    break;

  case 12:
    state.col = 0;
    state.lig = 0;
    state.ct = CONSOLE_WHITE;
    state.cf = CONSOLE_BLACK;
    screen_clean();
    break;

  case 13:
    state.col = 0;
    break;

  default: {
    uint16_t* const at = char_addr(state.col, state.lig);
    *at = pack_char(c >= 32 && c <= 126 ? c : ' ', state.ct, state.cf);
    state.col++;
    wrap_cursor();
    break;
  }
  }
  put_cursor(state.col, state.lig);
}

void console_putbytes(char *chaine, int32_t taille)
{
  for (int32_t i = 0; i < taille; i++) {
    handle_char(chaine[i]);
  }
}
void console_set_foreground(uint8_t c) { state.ct = c; }
void console_set_background(uint8_t c) { state.cf = c; }
