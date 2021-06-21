#include "syscall.h"
#include "stdio.h"
#include "start.h"

int test_run(int n);
void test_all() {
  for (int n = 1; n <= 20; n++) {
    printf("Test %d:\n", n);
    int pid = start((int (*)(void*))(void*)test_run, 4000, 128, "test", (void*)n);
    waitpid(pid, NULL);
  }
  printf("All done.\n");
}

void user_start() {
  test_all();
}
