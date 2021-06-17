#include "stddef.h"
#include "syscall.h"

/** Trigger user interrupt (in handlers.S) */
extern int SYS_call(int call_id, void* p1, void* p2, void* p3, void* p4, void* p5);

void console_putbytes(const char* str, int size) { SYS_call(0, (void*)str, (void*)size, NULL, NULL, NULL); }

int getprio(int pid) { return SYS_call(43, (void*)pid, NULL, NULL, NULL, NULL); }
