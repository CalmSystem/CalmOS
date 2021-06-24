#include "play.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "syscall.h"

enum notes { C, CD, D, DD, E, F, FD, G, GD, A, AD, B };
enum figures { RO, BP, BL, NP, NO, CP, CR, DC };
enum pauses { PA, DP, SO, DS, QS, HS };

const int low_notes[12] = {262, 277, 294, 311, 330, 349,
                           370, 392, 415, 440, 466, 494};
const int medium_notes[12] = {523, 554, 587, 622, 659, 698,
                              740, 784, 831, 880, 932, 988};
const int high_notes[12] = {1046, 1109, 1175, 1244, 1318, 1397,
                            1480, 1568, 1661, 1760, 1865, 1975};

float figures_fact[8] = {4, 3, 2, 1.5f, 1, 0.75f, 0.5f, 0.15f};
float pauses_fact[6] = {4, 2, 1, 0.5f, 0.25f, 0.125f};

struct music_notes {
  int note;
  int note_long;
  int delay;
};

int tempo = -1;

void play_note(char *line, unsigned long quartz, unsigned long ticks) {
  const int *scale;
  char note_char[3];
  int note;
  int accidental = 0;
  char figure_code[2];
  float figure;

  strncpy(note_char, line, 3);
  strncpy(figure_code, &line[4], 2);

  if (note_char[2] == '1')
    scale = low_notes;
  else if (note_char[2] == '2')
    scale = medium_notes;
  else
    scale = high_notes;

  if (note_char[1] == 'B')
    accidental = -1;
  else if (note_char[1] == 'D')
    accidental = 1;

  switch (note_char[0]) {
    case 'C':
      note = scale[C + accidental];
      break;
    case 'D':
      note = scale[D + accidental];
      break;
    case 'E':
      note = scale[E + accidental];
      break;
    case 'F':
      note = scale[F + accidental];
      break;
    case 'G':
      note = scale[G + accidental];
      break;
    case 'A':
      note = scale[A + accidental];
      break;
    case 'B':
      note = scale[B + accidental];
      break;
    default:
      note = -1;
      break;
  }

  if (note == -1) {
    printf("Incorrect file.\n");
    exit(-1);
  }

  switch (figure_code[0]) {
    case 'R':
      figure = figures_fact[RO];
      break;
    case 'B':
      if (figure_code[1] == 'L')
        figure = figures_fact[BL];
      else
        figure = figures_fact[BP];
      break;
    case 'N':
      if (figure_code[1] == 'O')
        figure = figures_fact[NO];
      else
        figure = figures_fact[NP];
      break;
    case 'C':
      if (figure_code[1] == 'R')
        figure = figures_fact[CR];
      else
        figure = figures_fact[CP];
      break;
    case 'D':
      figure = figures_fact[DC];
      break;
    default:
      figure = -1;
      break;
  }

  beep(note, ((60000 / tempo - 100) * figure) / 1000.);
  wait_clock(current_clock() + ((quartz / ticks) * (tempo / 1000.)));
}

void wait_silence(char *line, unsigned long quartz, unsigned long ticks) {
  char silent_code[2];
  float silent;
  memcpy(silent_code, &line[2], 2);
  switch (silent_code[0]) {
    case 'P':
      silent = pauses_fact[PA];
      break;
    case 'D':
      if (silent_code[1] == 'P')
        silent = pauses_fact[DP];
      else
        silent = pauses_fact[DS];
      break;
    case 'S':
      silent = pauses_fact[SO];
      break;
    case 'Q':
      silent = pauses_fact[QS];
      break;
    case 'H':
      silent = pauses_fact[HS];
      break;
    default:
      silent = -1;
      break;
  }

  wait_clock(current_clock() +
             ((quartz / ticks) *
              ((silent * (60000 / tempo) - (silent - 1) * 100) / 1000.)));
}

void decode_music_line(char *line) {
  unsigned long quartz, ticks;
  clock_settings(&quartz, &ticks);

  if (line[0] == 'T') {
    // printf("tempo setting\n");
    tempo = strtoul(&line[2], NULL, 10);
    return;
  }
  if (tempo == -1) {
    printf("Non set tempo.\n");
    return;
  }
  if (line[0] == 'S')
    wait_silence(line, quartz, ticks);
  else
    play_note(line, quartz, ticks);
}
