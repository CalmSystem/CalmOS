#include "stddef.h"
#include "syscall.h"

//NOTE: On our CPU, sizeof(int) == sizeof(long)

/** Trigger user interrupt (in handlers.S) */
extern int SYS_call(int call_id, void* p1, void* p2, void* p3, void* p4, void* p5);

#define SYS_call_0(iNum) SYS_call(iNum, NULL, NULL, NULL, NULL, NULL)
#define SYS_call_1(iNum, p1) SYS_call(iNum, (void *)p1, NULL, NULL, NULL, NULL)
#define SYS_call_2(iNum, p1, p2) SYS_call(iNum, (void *)p1, (void *)p2, NULL, NULL, NULL)
#define SYS_call_3(iNum, p1, p2, p3) SYS_call(iNum, (void *)p1, (void *)p2, (void *)p3, NULL, NULL)
#define SYS_call_4(iNum, p1, p2, p3, p4) SYS_call(iNum, (void *)p1, (void *)p2, (void *)p3, (void *)p4, NULL)
#define SYS_call_5(iNum, p1, p2, p3, p4, p5) SYS_call(iNum, (void *)p1, (void *)p2, (void *)p3, (void *)p4, (void *)p5)

void console_putbytes(const char *str, int size) { SYS_call_2(0, str, size); }

void cons_write(const char *str, unsigned long size) { SYS_call_2(10, str, size); }
int cons_read(void) { return SYS_call_0(11); }
void cons_echo(int on) { SYS_call_1(12, on); }
unsigned long cons_readline(char *string, unsigned long length) { return SYS_call_2(13, string, length); }
void beep(int freq, float delay) {
  // float to void* reinterpt cast
  union { float vf; void *vp; } cast;
  cast.vf = delay;
  SYS_call_2(14, freq, cast.vp);
}

int getpid(void) { return SYS_call_0(20); }
int waitpid(int pid, int *retval) { return SYS_call_2(21, pid, retval); }
int processes_status(struct process_status_t *status, int count) { return SYS_call_2(22, status, count); }

int chprio(int pid, int newprio) { return SYS_call_2(30, pid, newprio); }
int getprio(int pid) { return SYS_call_1(31, pid); }

int pcount(int fid, int *count) { return SYS_call_2(40, fid, count); }
int pcreate(int count) { return SYS_call_1(41, count); }
int pdelete(int fid) { return SYS_call_1(42, fid); }
int preceive(int fid, int *message) { return SYS_call_2(43, fid, message); }
int preset(int fid) { return SYS_call_1(44, fid); }
int psend(int fid, int message) { return SYS_call_2(45, fid, message); }
int queues_status(struct queue_status_t *status, int count) { return SYS_call_2(46, status, count); }

void clock_settings(unsigned long *quartz, unsigned long *ticks) { SYS_call_2(50, quartz, ticks); }
unsigned long current_clock(void) { return SYS_call_0(51); }
void wait_clock(unsigned long wakeup) { SYS_call_1(52, wakeup); }

int start(int (*ptfunc)(void *), unsigned long ssize, int prio,
          const char *name, void *arg) { return SYS_call_5(60, ptfunc, ssize, prio, name, arg); }
int kill(int pid) { return SYS_call_1(61, pid); }
void exit(int retval) {
  SYS_call_1(62, retval);
  while(1); //noreturn
}
void reboot() { SYS_call_0(63); }
