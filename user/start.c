#include "syscall.h"
#include "stdio.h"
#include "start.h"

void ping() {
  const int res = getprio(1);
  printf("Res is %d\n", res);
  for (int i = 0; i < 99; i++) { }
}

int child(void* arg) {
  printf("Got %d\n", (int)arg);
  return (int)arg * 2;
}

void user_start() {
  int pid = start(child, 800, 3, "child", (void*)5);
  int retval;
  int res = waitpid(pid, &retval);
  printf("%d ret %d(%d)\n", pid, retval, res);

  const unsigned long seconds = 2;
  unsigned long freq;
  clock_settings(NULL, &freq);
  while (1) {
    wait_clock(current_clock() + seconds * freq);
    printf("Ping every %lu seconds\n", seconds);
  }
}
