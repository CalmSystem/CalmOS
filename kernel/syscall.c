#include "stddef.h"
#include "start.h"
#include "console.h"
#include "scheduler.h"
#include "interrupt.h"
#include "queues.h"
#include "keyboard.h"
#include "beep.h"
#include "syscall.h"
#include "debug.h"

#define IS_USER_PTR(p) (p >= (void*)user_start && p < (void*)user_end)
//FIXME: process must segfault
#define SEGFAULT() return -42;
#define USER_PTR(p) \
if (!IS_USER_PTR(p)) SEGFAULT();
#define USER_OR_NULL_PTR(p) \
if (p != NULL) { USER_PTR(p); }

int user_IT(int call_id, void* p1, void* p2, void* p3, void* p4, void* p5) {
  switch (call_id) {
    case 0:
      USER_PTR(p1);
      console_putbytes((const char*)p1, (int)p2);
      return 0;

    case 10:
      USER_PTR(p1);
      return cons_write((const char*)p1, (long)p2);
    case 11:
      return cons_read();
    case 12:
      cons_echo((int)p1);
      return 0;
    case 13:
      USER_PTR(p1);
      return cons_readline((char*)p1, (unsigned long)p2);
    case 14:
      beep((int)p1, *(float*)&p2);
      return 0;

    case 20:
      return getpid();
    case 21:
      USER_OR_NULL_PTR(p2);
      return waitpid((int)p1, (int*)p2);

    case 30:
      return chprio((int)p1, (int)p2);
    case 31:
      return getprio((int)p1);

    case 40:
      USER_OR_NULL_PTR(p2);
      return pcount((int)p1, (int*)p2);
    case 41:
      return pcreate((int)p1);
    case 42:
      return pdelete((int)p1);
    case 43:
      USER_OR_NULL_PTR(p2);
      return preceive((int)p1, (int*)p2);
    case 44:
      return preset((int)p1);
    case 45:
      return psend((int)p1, (int)p2);

    case 50:
      USER_OR_NULL_PTR(p1);
      USER_OR_NULL_PTR(p2);
      clock_settings((unsigned long*)p1, (unsigned long*)p2);
      return 0;
    case 51:
      return current_clock();
    case 52:
      wait_clock((unsigned long)p1);
      return 0;

    case 60:
      USER_PTR(p1);
      USER_PTR(p4);
      return start_user((int (*)(void*))p1, (unsigned long)p2, (int)p3, (const char*)p4, p5);
    case 61:
      return kill((int)p1);
    case 62:
      exit((int)p1);
      return 0;

    default:
      SEGFAULT();
  }
}
