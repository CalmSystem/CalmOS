#include "debugger.h"
#include "stdio.h"
#include "logo.h"
#include "interrupt.h"
#include "cpu.h"
#include "scheduler.h"
#include "test.h"
#include "floppy.h"
#include "xxd.h"
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

int proc_flopped(void *arg) {
  char buffer[256];
  int ret = floppy_read(buffer, (uint32_t)arg, 256);
  printf("Got %d\n", ret);
  if (ret >= 0) hex_dump((size_t)arg, buffer, 256);
  return ret;
}

void kernel_start(void)
{
  // Splash screen
  printf(CALMOS_LOGO);

  // call_debugger(); useless with qemu -s -S
  setup_scheduler();
  setup_interrupt_handlers();

  // NOTE: Kernel tests
  // test_all();
  start_user_background(user_start, 4000, 1, "user_start", NULL);
  start_background(proc_flopped, 4000, 12, "proc_flopped", (void*)0x00005400);
  // start_background(proc_wait, 128, 1, "proc_fizz", (void*)3);
  // start_background(proc_wait, 128, 1, "proc_buzz", (void*)5);

  idle();

  printf("Bye\n");

	return;
}
