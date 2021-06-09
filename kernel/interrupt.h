#ifndef INTERRUPT_H_
#define INTERRUPT_H_

void setup_interrupt_handlers();
void tic_PIT();

void clock_settings(unsigned long *quartz, unsigned long *ticks);
unsigned long current_clock();

#define QUARTZ 0x1234DD
#define SCHEDFREQ 50
#define CLOCKFREQ 200
// ASSERT(CLOCKFREQ > SCHEDFREQ && CLOCKFREQ % SCHEDFREQ == 0)

#endif /*INTERRUPT_H_*/
