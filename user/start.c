#include "syscall.h"
#include "stdio.h"
#include "start.h"

void ping() {
  const int res = getprio(6);
  printf("Res is %d\n", res);
  for (int i = 0; i < 99; i++) { }
}

void user_start() {
  while (1) {
    ping();
  }
}
