#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "stdint.h"
#include "system.h"

/** Size of kernel process stack in int32_t */
#define NBSTACK 1024
/** Maximun size of user process stack in bytes */
#define MAXSTACK (1 << 30)

#define MINPRIO 1
#define MAXPRIO 256

struct process_t
{
  int pid;
  /** Parent process pid or NOPID */
  int parent;
  const char* name;
  int prio;
  enum process_state_t state;
  /** Discriminated union with state as tag (std::variant) */
  union {
    struct process_asleep_t {
      /** Asleep until clock */
      unsigned long clock;
      /** Next asleep process */
      struct process_t* next;
    } asleep;
    /** Zombie return value */
    int retval;
    /** Wait child pid */
    int* child;
    /** Next runnable process */
    struct process_t* next_runnable;
    struct {
      /** Next queue waiting process */
      struct process_t* next;
      /** Queue ID */
      int fid;
      /** Return value after waiting */
      int* retval;
      /** Message to send or receive */
      int* message;
    } wait_queue;
    /** Next dead process (free pid) */
    struct process_t* next_dead;
  } state_attr;
  int32_t registers[5];
  /** Kernel-space (Ring0) stack */
  int32_t kernel_stack[NBSTACK];
  /** Userspace (Ring3) stack */
  int32_t* user_stack;
  /** user_stack size in bytes */
  unsigned long ssize;
};

/** Initialize process table */
void setup_scheduler();
/** Lowest priority halt process */
void idle();
/** Trigger context_switch. Called by interrupt.c at SCHEDFREQ */
void tick_scheduler();
/** Trigger scheduler if current quantum must end */
void fix_scheduler();
/** Ptr to active process */
struct process_t* getproc();

int getpid(void);

int chprio(int pid, int newprio);
int getprio(int pid);
const char* getpname(int pid);

int start(int (*pt_func)(void *), unsigned long ssize, int prio,
          const char *name, void *arg);
/** Add process without triggering scheduler. Used to create initial processes */
int start_background(int (*pt_func)(void *), unsigned long ssize, int prio,
          const char *name, void *arg);
/** Add process in userspace */
int start_user(int (*pt_func)(void *), unsigned long ssize, int prio,
          const char *name, void *arg);
/** Add process in userspace without triggering scheduler. Used to create initial processes */
int start_user_background(int (*pt_func)(void *), unsigned long ssize, int prio,
          const char *name, void *arg);

void exit(int retval);
int kill(int pid);

void wait_clock(unsigned long clock);
int waitpid(int pid, int *retvalp);

void remove_runnable(struct process_t* ps);
void push_runnable(struct process_t* ps);

/** Get N firsts processes status. Returns total processes count */
int processes_status(struct process_status_t *status, int count);

#endif /*SCHEDULER_H_*/
