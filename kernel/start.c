#include "debugger.h"
#include "stdio.h"
#include "interrupt.h"
#include "cpu.h"
#include "scheduler.h"
#include "test.h"
#include "start.h"

#include "keyboard.h"

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

int proc_keyboard(void *arg) {
  while(1) {
    char str[10] = {0};
    int i = cons_readline(str, 10);
    printf("### Read: %s  (size: %d -> ", str, i);
    for(int j = 0; j < i; j++) {
      printf("%d,", str[j]);
    }
    printf(" ) ###\n");
  }
  return (int)arg;
}

void kernel_start(void)
{
  printf("\f"  // Splash screen
      "\t\t    ____      _            ___  ____  \n"
      "\t\t   / ___|__ _| |_ __ ___  / _ \\/ ___| \n"
      "\t\t  | |   / _` | | '_ ` _ \\| | | \\___ \\ \n"
      "\t\t  | |__| (_| | | | | | | | |_| |___) |\n"
      "\t\t   \\____\\__,_|_|_| |_| |_|\\___/|____/ \n"
      "\t\t     Keep Calm and Avoid Assembly\n\n");

  // call_debugger(); useless with qemu -s -S
  setup_scheduler();
  setup_interrupt_handlers();

  // NOTE: Kernel tests
  // test_all();
  start_user_background(user_start, 4000, 1, "user_start", NULL);
  // start_background(proc_keyboard, 4000, 12, "proc_keyboard", NULL);
  // start_background(proc_wait, 128, 1, "proc_fizz", (void*)3);
  // start_background(proc_wait, 128, 1, "proc_buzz", (void*)5);

  idle();

  printf("Bye\n");

	return;
}
