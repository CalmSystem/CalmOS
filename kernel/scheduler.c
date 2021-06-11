#include "cpu.h"
#include "stdint.h"
#include "stdbool.h"
#include "scheduler.h"
#include "interrupt.h"
#include "string.h"

enum process_state_t {
  /** Reusable */
  PS_DEAD = -2,
  /** Stopped and waiting parent */
  PS_ZOMBIE = -1,
  /** Can run */
  PS_RUNNABLE = 0,
  /** Currently running */
  PS_RUNNING,
  /** Waiting clock */
  PS_ASLEEP,
  /** Waiting child end */
  PS_WAIT_CHILD,
  /** Waiting semaphore */
  // TODO: PS_WAIT_LOCK,
  /** Waiting interrupt */
  // TODO: PS_WAIT_IO
};
struct process_t
{
  int pid;
  int parent;
  const char* name;
  int prio;
  enum process_state_t state;
  union {
    struct process_asleep_t {
      /** Asleep until clock */
      unsigned long clock;
      /** Next asleep process */
      struct process_t* next;
    } asleep;
    /** Zombie return value */
    int ret;
    /** Wait child pid */
    int* child;
    /** Next runnable process */
    struct process_t* next_runnable;
    /** Next dead process (free pid) */
    struct process_t* next_dead;
  } state_attr;
  int32_t registers[5];
  int32_t stack[NBSTACK];
};
struct process_t processes[NBPROC] = {0};
struct process_t* active_process;

struct process_t* runnable_process_head = NULL;
struct process_t* dead_process_head = NULL;
struct process_t* asleep_process_head = NULL;

extern void ctx_sw(int* save, int* restore);
void process_end();

int stop(int pid, int retval);

void remove_runnable(struct process_t* ps) {
  if (runnable_process_head != NULL && ps->state == PS_RUNNABLE) {
    if (runnable_process_head == ps) {
      runnable_process_head = runnable_process_head->state_attr.next_runnable;
    } else {
      struct process_t* node = runnable_process_head;
      while (node->state_attr.next_runnable != NULL) {
        if (node->state_attr.next_runnable == ps) {
          node->state_attr.next_runnable =
              node->state_attr.next_runnable->state_attr.next_runnable;
          break;
        }
        node = node->state_attr.next_runnable;
      }
    }
  }
}
void push_runnable(struct process_t* ps) {
  // assert(ps->state != PS_RUNNABLE)
  ps->state = PS_RUNNABLE;
  if (runnable_process_head == NULL || ps->prio > runnable_process_head->prio) {
    ps->state_attr.next_runnable = runnable_process_head;
    runnable_process_head = ps;
  } else {
    struct process_t* current = runnable_process_head;
    while (current->state_attr.next_runnable != NULL &&
           ps->prio <= current->state_attr.next_runnable->prio) {
      current = current->state_attr.next_runnable;
    }
    ps->state_attr.next_runnable = current->state_attr.next_runnable;
    current->state_attr.next_runnable = ps;
  }
}

void push_dead(struct process_t* ps) {
  // assert(ps->state != PS_DEAD);
  ps->state = PS_DEAD;
  ps->state_attr.next_dead = dead_process_head;
  dead_process_head = ps;
}

int getpid() { return active_process->pid; }
#define IS_VALID_PID(pid) \
(pid >= 0 && pid < NBPROC && processes[pid].state != PS_DEAD)
#define VALID_PID(pid) \
if (!IS_VALID_PID(pid)) return NOPID;
#define ALIVE_PID(pid) \
if (pid < 0 || pid >= NBPROC || processes[pid].state < PS_RUNNABLE) return NOPID;

void fix_scheduler() {
  if (active_process->state != PS_RUNNING ||
  (runnable_process_head != NULL && active_process->prio < runnable_process_head->prio))
    tick_scheduler();
}

int chprio(int pid, int newprio) {
  ALIVE_PID(pid);
  if (newprio <= 0 || newprio > MAXPRIO) return -2;

  struct process_t* const ps = &processes[pid];
  if (ps->prio == newprio) return newprio;

  const int oldprio = ps->prio;
  ps->prio = newprio;

  if (ps->state == PS_RUNNABLE) {
    remove_runnable(ps);
    push_runnable(ps);
  }
  fix_scheduler();
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

int start_background(int (*pt_func)(void*), unsigned long ssize, int prio,
          const char* name, void* arg) {
  if (ssize / sizeof(int32_t) > NBSTACK) return -2;

  if (dead_process_head == NULL) return NOPID;
  struct process_t* const ps = dead_process_head;
  dead_process_head = dead_process_head->state_attr.next_dead;
  // assert(ps->state == PS_DEAD);

  ps->name = name;
  ps->prio = prio;
  ps->parent = getpid();
  ps->stack[NBSTACK - 3] = (int32_t)pt_func;
  ps->stack[NBSTACK - 2] = (int32_t)&process_end;
  ps->stack[NBSTACK - 1] = (int32_t)arg;
  ps->registers[1] = (int32_t)&ps->stack[NBSTACK - 3];
  push_runnable(ps);
  return ps->pid;
}
int start(int (*pt_func)(void*), unsigned long ssize, int prio,
          const char* name, void* arg) {
  const int pid = start_background(pt_func, ssize, prio, name, arg);
  fix_scheduler();
  return pid;
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
int kill(int pid) {
  if (pid <= 0) return NOPID;
  return stop(pid, 0);
  }

void wait_clock(unsigned long clock)
{
  const int pid = getpid();
  struct process_t* const ps = &processes[pid];
  remove_runnable(ps);
  ps->state = PS_ASLEEP;
  ps->state_attr.asleep.clock = clock;

  // Add to asleep queue
  if (asleep_process_head == NULL || clock <= asleep_process_head->state_attr.asleep.clock) {
    ps->state_attr.asleep.next = asleep_process_head;
    asleep_process_head = ps;
  } else {
    struct process_asleep_t* prev = &asleep_process_head->state_attr.asleep;
    while (prev->next != NULL && clock > prev->next->state_attr.asleep.clock) {
      prev = &prev->next->state_attr.asleep;
    }
    ps->state_attr.asleep.next = prev->next;
    prev->next = ps;
  }
  tick_scheduler();
}
int waitpid(int pid, int* retvalp)
{
  if (pid >= 0) { VALID_PID(pid); }
  struct process_t* const ps = &processes[getpid()];
  struct process_t* child = NULL;
  if (pid >= 0) {
    VALID_PID(pid);
    if (processes[pid].parent != ps->pid) return -2;
    if (processes[pid].state == PS_ZOMBIE) child = &processes[pid];
  } else {
    bool has_child = false;
    // MAYBE: add child list
    for (int i = 0; child == NULL && i < NBPROC; i++) {
      if (processes[i].parent == ps->pid && processes[i].state > PS_DEAD) {
        has_child = true;
        if (processes[i].state == PS_ZOMBIE) child = &processes[i];
      }
    }
    if (child == NULL && !has_child) return -2;
  }
  if (child == NULL) {
    remove_runnable(ps);
    int child_pid = pid;
    ps->state = PS_WAIT_CHILD;
    ps->state_attr.child = &child_pid;
    tick_scheduler();
    // NOTE: child writes its pid in child_pid
    child = &processes[child_pid];
    // assert(child != NULL && child->state == PS_ZOMBIE);
  }
  if (retvalp) *retvalp = child->state_attr.ret;
  push_dead(child);
  return child->pid;
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
    processes[i].state_attr.next_dead = &processes[i+1];
  }
  processes[NBPROC - 1].state_attr.next_dead = NULL;
  dead_process_head = &processes[1];

  processes[0].prio = 0;
  processes[0].parent = NOPID;
  processes[0].name = "idle";
  processes[0].state = PS_RUNNING;
  active_process = &processes[0];
}

/** Any process termination (kill, exit, return) */
int stop(int pid, int retval) {
  ALIVE_PID(pid)
  struct process_t* const ps = &processes[pid];
  // Unbind children
  //MAYBE: use child list
  for (int i = 0; i < NBPROC; i++) {
    struct process_t* const child = &processes[i];
    if (child->parent != ps->pid) continue;
    if (child->state == PS_ZOMBIE) {
      push_dead(child);
    } else {
      child->parent = NOPID;
    }
  }

  if (ps->state == PS_ASLEEP && asleep_process_head != NULL) {
    // Remove from wait clock queue
    if (ps == asleep_process_head) {
      asleep_process_head = ps->state_attr.asleep.next;
    } else {
      struct process_asleep_t* prev = &asleep_process_head->state_attr.asleep;
      while (prev->next != NULL) {
        if (prev->next == ps) {
          prev->next = ps->state_attr.asleep.next;
          break;
        }
        prev = &prev->next->state_attr.asleep;
      }
    }
  }

  remove_runnable(ps);
  if (ps->parent == NOPID) {
    push_dead(ps);
  } else {
    ps->state = PS_ZOMBIE;
    ps->state_attr.ret = retval;
    // Trigger waiting parent
    struct process_t* const parent = &processes[ps->parent];
    if (parent->state == PS_WAIT_CHILD && (*parent->state_attr.child == NOPID ||
                                           *parent->state_attr.child == pid)) {
      *parent->state_attr.child = pid;
      push_runnable(parent);
    }
  }
  fix_scheduler();
  return retval;
}

/** Change running process */
void tick_scheduler() {
  {  // Wake up processes on PS_ASLEEP
    unsigned long time = current_clock();
    while (asleep_process_head != NULL && time >= asleep_process_head->state_attr.asleep.clock) {
      // assert(asleep_process_head->state == PS_ASLEEP);
      struct process_t* const awake = asleep_process_head;
      asleep_process_head = asleep_process_head->state_attr.asleep.next;
      push_runnable(awake);
    }
  }

  if (active_process->state != PS_RUNNING || (runnable_process_head != NULL && runnable_process_head->prio >= active_process->prio)) {
    struct process_t* prev_process = active_process;
    active_process = runnable_process_head;
    // Pop runnable
    runnable_process_head = runnable_process_head->state_attr.next_runnable;
    
    if (prev_process->state == PS_RUNNING) {
      push_runnable(prev_process);
    }
    active_process->state = PS_RUNNING;
    ctx_sw(prev_process->registers, active_process->registers);
  }
}