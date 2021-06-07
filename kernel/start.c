#include "debugger.h"
#include "cpu.h"
#include "stdio.h"
#include "console.h"

int fact(int n)
{
	if (n < 2)
		return 1;

	return n * fact(n-1);
}


void kernel_start(void)
{
	int i;
	// call_debugger(); useless with qemu -s -S

	printf("\f");
	printf("Hello World é !\n");
	console_set_background(CONSOLE_RED);
	console_set_foreground(CONSOLE_LIGHT_CYAN);
	i = 10;
	printf("Fact %d is ", i);

	i = fact(i);
	printf("%d\n", i);

  console_set_background(CONSOLE_BLACK);
  console_set_foreground(CONSOLE_WHITE);
	printf("Bye\n");

  while(1) {}

	return;
}
