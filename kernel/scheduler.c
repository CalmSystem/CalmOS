#include "cpu.h"
#include "stdint.h"
#include "scheduler.h"

#include "stdio.h"

enum process_state_t
{
  PS_DEAD = -2,
  PS_ZOMBIE = -1,
  PS_RUNNABLE = 0,
  PS_RUNNING,
  PS_ASLEEP,
  PS_WAIT_LOCK,
  PS_WAIT_IO,
  PS_WAIT_CHILD
};
struct process_t
{
  int pid;
  int parent;
  const char* name;
  int prio;
  enum process_state_t state;
  union {
    /** Asleep until clock */
    unsigned long sleep;
    /** Zombie return value */
    int ret;
    /** Wait child pid */
    int child;
  } state_attr;
  int32_t registers[5];
  int32_t stack[NBSTACK];
};
struct process_t processes[NBPROC] = {0};
struct process_t* active_process;

extern void ctx_sw(int* save, int* restore);
void process_end();

void schedule();
int stop(int pid, int retval);

int getpid() { return active_process->pid; }
#define IS_VALID_PID(pid) \
(pid >= 0 && pid < NBPROC && processes[pid].state != PS_DEAD)
#define VALID_PID(pid) \
if (!IS_VALID_PID(pid)) return NOPID;
#define ALIVE_PID(pid) \
if (pid < 0 || pid >= NBPROC || processes[pid].state < PS_RUNNABLE) return NOPID;

int chprio(int pid, int newprio) {
  ALIVE_PID(pid);
  int oldprio = processes[pid].prio;
  processes[pid].prio = newprio;
  schedule();
  return oldprio;
}
int getprio(int pid) {
  ALIVE_PID(pid);
  return processes[pid].prio;
}
const char* getpname(int pid) {
  if (!IS_VALID_PID(pid)) return 0;
  return processes[pid].name;
}

int start(int (*pt_func)(void*), unsigned long ssize, int prio,
          const char* name, void* arg) {
  if (ssize > NBSTACK) return -2;

  for (int i = 0; i < NBPROC; i++) {
    if (processes[i].state != PS_DEAD) continue;

    processes[i].name = name;
    processes[i].prio = prio;
    processes[i].parent = getpid();
    processes[i].state = PS_RUNNABLE;
    processes[i].stack[NBSTACK - 3] = (int32_t)pt_func;
    processes[i].stack[NBSTACK - 2] = (int32_t)&process_end;
    processes[i].stack[NBSTACK - 1] = (int32_t)arg;
    processes[i].registers[1] = (int32_t)&processes[i].stack[NBSTACK - 3];
    return i;
  }
  return NOPID;
}

void process_end() {
  int retval = 0;
  __asm__ __volatile__("\t movl %%eax,%0" : "=r"(retval));
  exit(retval);
}
void exit(int retval)
{
  stop(getpid(), retval);
  while(1); //noreturn
}
int kill(int pid) { return stop(pid, 0); }

void wait_clock(unsigned long clock)
{
  struct process_t* const ps = &processes[getpid()];
  ps->state = PS_ASLEEP;
  ps->state_attr.sleep = clock;
  //TODO: add to wait queue
  schedule();
}
int waitpid(int pid, int* retvalp)
{
  if (pid >= 0) { VALID_PID(pid); }
  struct process_t* const ps = &processes[getpid()];
  ps->state = PS_WAIT_CHILD;
  ps->state_attr.child = pid;
  // TODO: add to wait queue
  schedule();
  if (retvalp) {
    struct process_t* const child = &processes[ps->state_attr.child];
    // assert(child->state == PS_ZOMBIE);
    *retvalp = child->state_attr.ret;
    child->state = PS_DEAD;
  }
  return ps->state_attr.child;
}

void idle(void) {
  for (;;) {
    sti();
    hlt();
    cli();
  }
}

void setup_scheduler()
{
  for (int i = 0; i < NBPROC; i++) {
    processes[i].pid = i;
    processes[i].state = PS_DEAD;
  }
  processes[0].prio = 0;
  processes[0].parent = NOPID;
  processes[0].name = "idle";
  processes[0].state = PS_RUNNING;
  active_process = &processes[0];
}

void tick_scheduler() { schedule(); }

/** Any process termination (kill, exit, return) */
int stop(int pid, int retval) {
  ALIVE_PID(pid)
  struct process_t* const ps = &processes[pid];
  // Unbind children
  for (int i = 0; i < NBPROC; i++) {
    struct process_t* const child = &processes[i];
    if (child->parent != ps->pid) continue;
    if (child->state == PS_ZOMBIE) {
      child->state = PS_DEAD;
    } else {
      child->parent = NOPID;
    }
  }
  if (ps->parent == NOPID) {
    ps->state = PS_DEAD;
  } else {
    ps->state = PS_ZOMBIE;
    ps->state_attr.ret = retval;
    // Trigger waiting parent
    struct process_t* const parent = &processes[ps->parent];
    if (parent->state == PS_WAIT_CHILD && (parent->state_attr.child == NOPID ||
                                           parent->state_attr.child == pid)) {
      parent->state = PS_RUNNABLE;
      parent->state_attr.child = pid;
    }
  }
  // TODO: remove from wait queues
  printf("[%s(%d)] end with %d\n", ps->name, ps->pid, retval);
  schedule();
  return retval;
}

/** Change running process */
void schedule() {
  // TODO: check wait queues

  struct process_t* prev_process = active_process;
  for (int i = 1; i < NBPROC && active_process == prev_process; i++) {
    int pid = (prev_process->pid + i) % NBPROC;
    if (processes[pid].state == PS_RUNNABLE)
      active_process = &processes[pid];
  }
  if (prev_process->state == PS_RUNNING) {
    prev_process->state = PS_RUNNABLE;
  }
  active_process->state = PS_RUNNING;
  ctx_sw(prev_process->registers, active_process->registers);
}