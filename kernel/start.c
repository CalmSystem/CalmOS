#include "debugger.h"
#include "stdio.h"
#include "interrupt.h"
#include "cpu.h"
#include "scheduler.h"

int proc_end(void* arg) {
  unsigned long i, j;
  for (j = 0; j < 9; j++) {
    printf("%d\n", (int)arg); /* l'autre processus doit afficher des 'B' */
    for (i = 0; i < 500000; i++){}
		sti(); /* demasquage des interruptions */
		/* une ou plusieurs it du timer peuvent survenir pendant cette boucle d'attente */
		for (i = 0; i < 500000; i++);
		cli(); /* masquage des interruptions */	
	}
  return (int)arg * 2;
}
int proc_inf(void* arg) {
  unsigned long i;
  while(1) {
    printf("{%d}", (int)arg); /* l'autre processus doit afficher des 'B' */
    for (i = 0; i < 500000; i++){}
		sti(); /* demasquage des interruptions */
		/* une ou plusieurs it du timer peuvent survenir pendant cette boucle d'attente */
		for (i = 0; i < 500000; i++);
		cli(); /* masquage des interruptions */	
	}
}


void kernel_start(void)
{
  // call_debugger(); useless with qemu -s -S
  setup_scheduler();
  setup_interrupt_handlers();

  start(proc_end, 128, 1, "proc_end3", (void*)3);
  start(proc_end, 128, 1, "proc_end7", (void*)7);
  start(proc_inf, 128, 1, "proc_inf", (void*)42);
  idle();

  printf("Bye\n");

	return;
}
