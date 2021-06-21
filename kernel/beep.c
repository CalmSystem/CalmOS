#include "interrupt.h"
#include "scheduler.h"
#include "cpu.h"
#include "beep.h"

void beep(int freq, float delay) {
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

  unsigned long quartz;
  unsigned long ticks;
  clock_settings(&quartz, &ticks);
  wait_clock(current_clock() + (quartz / ticks) * delay);

  tmp = inb(0x61) & 0xFC;
  outb(tmp, 0x61);
}