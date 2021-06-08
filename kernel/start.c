#include "debugger.h"
#include "stdio.h"
#include "interrupt.h"
#include "scheduler.h"


void kernel_start(void)
{
  setup_scheduler();
  //setup_interrupt_handlers();

  // call_debugger(); useless with qemu -s -S

  idle();

  printf("Bye\n");

	return;
}
