#include "segment.h"
#include "stdint.h"
#include "stdbool.h"
#include "stdio.h"
#include "cpu.h"
#include "console.h"
#include "interrupt.h"

void* const IDT = (void*)0x1000;
/** Mask table for IRQ 0 to 7. Handled with interrupt 32-47 */
const uint32_t IRQ_LOW_MASK_DATA_PORT = 0x21;
// IRQ_HIGH_MASK_DATA_PORT 0xA1

extern void traitant_IT_32();

void set_handler(unsigned int nidt, void (*handler)(void)) {
  uint16_t* const cell = IDT + nidt * 8;
  *(cell + 0) = (unsigned int)handler;
  *(cell + 1) = KERNEL_CS;
  *(cell + 2) = 0x8E00;
  *(cell + 3) = (unsigned int)handler >> 16;
}

void set_mask(uint8_t irq, bool masked)
{
  uint8_t masks = inb(IRQ_LOW_MASK_DATA_PORT);
  // Set bit irq to masked
  masks = (masks & ~(1UL << irq)) | (masked << irq);
  outb(masks, IRQ_LOW_MASK_DATA_PORT);
}

void set_pit()
{
  const int t = 0x1234DD / CLOCKFREQ;
  outb(0x34, 0x43);
  outb(t % 256, 0x40);
  outb(t / 256, 0x40);
}
unsigned long pit_count = 0;
void tic_PIT() {
  outb(0x20, 0x20);
  pit_count++;
  unsigned long seconds = pit_count / CLOCKFREQ;
  char timeStr[9]; // space for "HH:MM:SS\0"
  sprintf(timeStr, "%02ld:%02ld:%02ld", seconds / (60 * 60), (seconds / 60) % 60, seconds % 60);
  console_putbytes_at(timeStr, 8, CONSOLE_COL-8, 0);
}

void setup_interrupt_handlers()
{
  set_handler(32, traitant_IT_32);
  set_pit();
  set_mask(0, false);
  sti();
}