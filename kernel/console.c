#include "cpu.h"
#include "stdint.h"
#include "console.h"
#include "string.h"
#include "beep.h"
#include "system.h"

uint16_t* const DISPLAY_START = (uint16_t*)0xB8000;
const uint16_t CURSOR_PORT_CMD = 0x3D4;
const uint16_t CURSOR_PORT_DATA = 0x3D5;

/** Cursor state */
struct state_t {
  uint8_t col, lig;
  uint8_t ct, cf;
} state = {0, 0, CONSOLE_WHITE, CONSOLE_BLACK};

/** Address of a cell */
static uint16_t* char_addr(uint8_t col, uint8_t lig) {
  return DISPLAY_START + (lig * CONSOLE_COL + col);
}
/** Group character and style */
static uint16_t pack_char(uint8_t text, uint8_t ct, uint8_t cf) {
  const uint8_t CL = 0;
  uint16_t color = (CL << 7) | (cf << 4) | (ct);
  return (color << 8) | (text);
}
/** Writes cursor position */
static void put_cursor(uint8_t col, uint8_t lig) {
  const uint16_t at = col + lig * CONSOLE_COL;
  outb(0x0F, CURSOR_PORT_CMD);
  outb(at, CURSOR_PORT_DATA);
  outb(0x0E, CURSOR_PORT_CMD);
  outb(at >> 8, CURSOR_PORT_DATA);
}

/** Clear screen buffer part */
static void screen_flush(uint16_t from, uint16_t to) {
  const uint16_t clean = pack_char(' ', CONSOLE_WHITE, CONSOLE_BLACK);
  for (uint16_t i = from; i < to; i++) {
    *(DISPLAY_START + i) = clean;
  }
}
/** Clear whole screen buffer */
void screen_clean() { screen_flush(0, CONSOLE_LIG * CONSOLE_COL); }
/** Move screen buffer up */
void screen_scroll() {
  const uint16_t bottom = (CONSOLE_LIG - 1) * CONSOLE_COL;
  memmove(DISPLAY_START, DISPLAY_START + CONSOLE_COL, sizeof(uint16_t) * bottom);
  screen_flush(bottom, CONSOLE_LIG * CONSOLE_COL);
}

/** Move cursor back to CONSOLE_COL, CONSOLE_LIG */
void wrap_cursor() {
  if (state.col >= CONSOLE_COL) {
    state.col = 0;
    state.lig++;
  }
  if (state.lig >= CONSOLE_LIG) {
    screen_scroll();
    state.lig = CONSOLE_LIG - 1;
  }
}
/** Display or change state with a single char */
void handle_char(char c) {
  switch (c)
  {
  case 7: //beep
    beep(1000, .1f);
    break;

  case 8: //back
    if (state.col > 0) state.col--;
    break;

  case 9: //tab
    state.col = ((state.col / 8) + 1) * 8;
    if (state.col >= CONSOLE_COL) state.col = CONSOLE_COL-1;
    break;

  case 10: //line break
    state.col = 0;
    state.lig++;
    wrap_cursor();
    break;

  case 12: //flush
    state.col = 0;
    state.lig = 0;
    state.ct = CONSOLE_WHITE;
    state.cf = CONSOLE_BLACK;
    screen_clean();
    break;

  case 13: //carriage return
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
}

void console_putbytes(const char *chaine, int32_t taille) {
  for (int32_t i = 0; i < taille; i++) {
    handle_char(chaine[i]);
  }
  put_cursor(state.col, state.lig);
}
void console_putbytes_at(const char *chaine, int32_t taille, uint8_t col, uint8_t lig) {
  struct state_t save = state;
  state.col = col;
  state.lig = lig;
  state.ct = CONSOLE_WHITE;
  state.cf = CONSOLE_BLACK;
  console_putbytes(chaine, taille);
  state = save;
  put_cursor(state.col, state.lig);
}

void console_draw_red_cross() {
  struct state_t save = state;
  state.col = REDCROSS_COL;
  state.lig = REDCROSS_LIG;
  state.ct = CONSOLE_WHITE;
  state.cf = CONSOLE_RED;
  console_putbytes("X", 1);
  state = save;
  put_cursor(state.col, state.lig);
}

void console_set_background_at(uint8_t col, uint8_t lig, uint8_t c) {
  uint16_t* cell = char_addr(col, lig);
  *cell &= 0b1000111111111111;
  *cell |= (c << 12);
}

void console_set_foreground(uint8_t c) { state.ct = c; }
void console_set_background(uint8_t c) { state.cf = c; }
