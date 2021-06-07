#include "debugger.h"
#include "cpu.h"
#include "stdio.h"
#include "console.h"
#include "interrupt.h"

int fact(int n)
{
	if (n < 2)
		return 1;

	return n * fact(n-1);
}


void kernel_start(void)
{
  setup_interrupt_handlers();

  int i;
  // call_debugger(); useless with qemu -s -S

  i = 10;
  printf("Fact %d is ", i);

  i = fact(i);
  printf("%d\n", i);

  printf("Bye\n");

  while(1) hlt();

	return;
}
