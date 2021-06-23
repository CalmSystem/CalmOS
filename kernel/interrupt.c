#include "segment.h"
#include "stdint.h"
#include "stdbool.h"
#include "stdio.h"
#include "cpu.h"
#include "console.h"
#include "scheduler.h"
#include "kbd.h"
#include "keyboard.h"
#include "beep.h"
#include "interrupt.h"

#include "ps2.h"
#include "mouse.h"

void* const IDT = (void*)0x1000;
/** Mask table for IRQ 0 to 7. Handled with interrupt 32-47 */
const uint32_t IRQ_LOW_MASK_DATA_PORT = 0x21;
/** Mask table for IRQ 8 to 15. */
const uint32_t IRQ_HIGH_MASK_DATA_PORT = 0xA1;

extern void IT_PIT_handler();
extern void IT_KEYBOARD_handler();
extern void IT_MOUSE_handler();
extern void IT_USR_handler();

/** Writes handler on IDT */
void set_handler(unsigned int nidt, void (*handler)(void), uint8_t ring_level) {
  uint16_t* const cell = IDT + nidt * 8;
  *(cell + 0) = (unsigned int)handler;
  *(cell + 1) = KERNEL_CS;
  *(cell + 2) = 0x8E00 | ((uint16_t)ring_level << 13);
  *(cell + 3) = (unsigned int)handler >> 16;
}

/** Writes mask for IRQ (only between 0 and 7) */
void set_mask(uint8_t irq, bool masked) {
  uint32_t dataport = 0;
  if(irq >= 8) {
    dataport = IRQ_HIGH_MASK_DATA_PORT;
    irq -= 8;
  } else {
    dataport = IRQ_LOW_MASK_DATA_PORT;
  }
  uint8_t masks = inb(dataport);
  // Set bit irq to masked
  masks = (masks & ~(1UL << irq)) | (masked << irq);
  outb(masks, dataport);
}

/** Writes programmable clock interval */
void set_pit() {
  const int interval = QUARTZ / CLOCKFREQ;
  outb(0x34, 0x43);
  outb(interval % 256, 0x40);
  outb(interval / 256, 0x40);
}
unsigned long pit_count = 0;
void tic_PIT() {
  outb(0x20, 0x20);
  pit_count++;
  const unsigned long seconds = pit_count / CLOCKFREQ;
  char time_str[9]; // space for "HH:MM:SS\0"
  sprintf(time_str, "%02ld:%02ld:%02ld", seconds / (60 * 60), (seconds / 60) % 60, seconds % 60);
  console_putbytes_at(time_str, 8, CONSOLE_COL-8, 0);

  check_buzzer();

  if (pit_count % (CLOCKFREQ / SCHEDFREQ) == 0) {
    tick_scheduler();
  }
}

void keyboard_IT(){
  outb(0x20, 0x20);
  char c = inb(0x60);
  // printf("- %c -", inb(0x64));
  do_scancode((int)c);
}

void clock_settings(unsigned long* quartz, unsigned long* ticks) {
  if (quartz != NULL) *quartz = QUARTZ;
  if (ticks != NULL) *ticks = (QUARTZ / CLOCKFREQ);
}
unsigned long current_clock() { return pit_count; }

void setup_interrupt_handlers() {
  if (!(CLOCKFREQ > SCHEDFREQ && CLOCKFREQ % SCHEDFREQ == 0)) panic("Invalid clock constants");
  set_handler(32, IT_PIT_handler, 0);
  set_handler(33, IT_KEYBOARD_handler, 0);

  // mouse_init();
  init_ps2();

  set_handler(44, IT_MOUSE_handler, 0);

  set_pit();

  set_mask(0, false);
  set_mask(1, false);

  // Masking IRQ2 will cause the Slave PIC to stop raising IRQs.
  // https://wiki.osdev.org/8259_PIC
  // SO: unmask !
  set_mask(2, false);
  set_mask(12, false);

  set_handler(49, IT_USR_handler, 3);

  init_keyboard_buffer();
}