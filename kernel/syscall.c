#include "console.h"
#include "stdio.h"
#include "syscall.h"

int user_IT(int call_id, void* p1, void* p2, void* p3, void* p4, void* p5) {
  printf("Got SYS_call %d (%p-%p-%p-%p-%p)\n", call_id, p1, p2, p3, p4, p5);
  switch (call_id) {
    case 0:
      console_putbytes((const char*)p1, (int)p2);
      return 0;

    case 43:  // getprio
      return (int)p1 * 2;

    default:
      return -1;
  }
}
