#ifndef SCHEDULER_H_
#define SCHEDULER_H_

void setup_scheduler();
void idle();
void tick_scheduler();

int getpid(void);

int chprio(int pid, int newprio);
int getprio(int pid);
const char* getpname(int pid);

int start(int (*pt_func)(void *), unsigned long ssize, int prio,
          const char *name, void *arg);
void exit(int retval);
int kill(int pid);

void wait_clock(unsigned long clock);
int waitpid(int pid, int *retvalp);

#define NOPID -1

#define NBPROC 30
#define NBSTACK 1024

#define MINPRIO 1
#define MAXPRIO 256

#endif /*SCHEDULER_H_*/
