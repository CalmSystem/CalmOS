#include "debugger.h"
#include "stdio.h"
#include "logo.h"
#include "interrupt.h"
#include "cpu.h"
#include "scheduler.h"
#include "filesystem.h"
#include "test.h"
#include "start.h"

int proc_wait(void* arg) {
  const unsigned long seconds = (unsigned long)arg;
  unsigned long quartz;
  unsigned long ticks;
  clock_settings(&quartz, &ticks);
  while (1) {
    wait_clock(current_clock() + seconds * (quartz / ticks));
    printf("Ping every %lu seconds\n", seconds);
  }
}

void kernel_start(void)
{
  // Splash screen
  printf(CALMOS_LOGO);

  setup_scheduler();
  setup_interrupt_handlers();
  setup_filesystem();

  // NOTE: Kernel tests
  // test_all();
  start_user_background(user_start, 4000, 1, "user_start", NULL);
  // start_background(proc_wait, 128, 1, "proc_fizz", (void*)3);
  // start_background(proc_wait, 128, 1, "proc_buzz", (void*)5);

  idle();

  printf("Bye\n");

	return;
}
