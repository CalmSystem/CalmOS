#include "interrupt.h"
#include "scheduler.h"
#include "cpu.h"
#include "beep.h"

/** Zero or tick when buzzer must stop */
unsigned long beeping_end = 0;

void start_beep(int freq) {
  // Set the PIT to the desired frequency
  uint16_t div = 1193180 / freq;
  outb(0xb6, 0x43);
  outb(div, 0x42);
  outb(div >> 8, 0x42);

  // And play the sound using the PC speaker
  uint8_t tmp = inb(0x61);
  if (tmp != (tmp | 3)) {
    outb(tmp | 3, 0x61);
  }
}
void stop_beep() {
  uint8_t tmp = inb(0x61) & 0xFC;
  outb(tmp, 0x61);
}

unsigned long end_tick(float delay) {
  unsigned long quartz;
  unsigned long ticks;
  clock_settings(&quartz, &ticks);
  return current_clock() + (unsigned long)((quartz / ticks) * delay);
}

void beep(int freq, float delay) {
  start_beep(freq);
  wait_clock(end_tick(delay));
  stop_beep();
}

void async_beep(int freq, float delay) {
  start_beep(freq);
  beeping_end = end_tick(delay);
}

void check_buzzer() {
  if (beeping_end && current_clock() >= beeping_end) {
    stop_beep();
    beeping_end = 0;
  }
}
