#include "cpu.h"
#include "stdbool.h"
#include "scheduler.h"
#include "segment.h"
#include "string.h"
#include "user_stack_mem.h"
#include "debug.h"
#include "interrupt.h"
#include "queues.h"

struct process_t processes[NBPROC] = {0};
/** Currently running process */
struct process_t* active_process;

/** Linked list of runnable processes sorted by descressing priority. Next is state_attr.next_runnable */
struct process_t* runnable_process_head = NULL;
/** Stack of dead processes aka reusable pid. Next is state_attr.next_dead */
struct process_t* dead_process_head = NULL;
/** Linked list of asleep processes sorted by ascending wake time (clock). Next is state_attr.asleep.next */
struct process_t* asleep_process_head = NULL;

/** Context switch assembly */
extern void CTX_swap(int* save, int* restore);
/** Calls stop using captured return value */
extern void PROC_end();
/** Switch to usermode with iret */
extern void JMP_usermode();

/** Any process termination (kill, exit, return) */
int stop(int pid, int retval);

/** Remove process from runnable list */
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
/** Add process to runnable list */
void push_runnable(struct process_t* ps) {
  assert(ps->state != PS_RUNNABLE);
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
/** Add process to dead stack */
void push_dead(struct process_t* ps) {
  assert(ps->state != PS_DEAD);
  if (ps->user_stack != NULL) {
    user_stack_free(ps->user_stack, ps->ssize);
    ps->user_stack = NULL;
  }
  ps->state = PS_DEAD;
  ps->state_attr.next_dead = dead_process_head;
  dead_process_head = ps;
}

int getpid() { return active_process->pid; }
struct process_t* getproc() { return &processes[getpid()]; }

#define IS_VALID_PID(pid) \
(pid >= 0 && pid < NBPROC && processes[pid].state != PS_DEAD)
#define VALID_PID(pid) \
if (!IS_VALID_PID(pid)) return NOPID;
#define ALIVE_PID(pid) \
if (pid < 0 || pid >= NBPROC || processes[pid].state < PS_RUNNABLE) return NOPID;

int chprio(int pid, int newprio) {
  ALIVE_PID(pid);
  if (newprio <= 0 || newprio > MAXPRIO) return -2;

  struct process_t* const ps = &processes[pid];
  if (ps->prio == newprio) return newprio;

  const int oldprio = ps->prio;
  ps->prio = newprio;

  switch (ps->state)
  {
  case PS_RUNNABLE:
    remove_runnable(ps);
    // Process state is temporary undefined
    ps->state = (enum process_state_t) - 1;
    push_runnable(ps);
    break;
  
  case PS_WAIT_QUEUE_EMPTY:
    queue_reorder_empty_process(ps);
    break;

  case PS_WAIT_QUEUE_FULL:
    queue_reorder_full_process(ps);
    break;

  default:
    break;
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
  assert(ps->state == PS_DEAD);

  ps->name = name;
  ps->prio = prio;
  ps->parent = getpid();
  ps->ssize = 0;
  //NOTE: no user_stack for kernel process
  ps->user_stack = NULL;
  ps->kernel_stack[NBSTACK - 3] = (int32_t)pt_func;
  ps->kernel_stack[NBSTACK - 2] = (int32_t)&PROC_end;
  ps->kernel_stack[NBSTACK - 1] = (int32_t)arg;
  ps->registers[1] = (int32_t)&ps->kernel_stack[NBSTACK - 3];
  push_runnable(ps);
  return ps->pid;
}
int start(int (*pt_func)(void*), unsigned long ssize, int prio,
          const char* name, void* arg) {
  const int pid = start_background(pt_func, ssize, prio, name, arg);
  fix_scheduler();
  return pid;
}

int start_user_background(int (*pt_func)(void*), unsigned long ssize, int prio,
                     const char* name, void* arg) {
  if (ssize > MAXSTACK) return -2;

  if (dead_process_head == NULL) return NOPID;
  struct process_t* const ps = dead_process_head;
  dead_process_head = dead_process_head->state_attr.next_dead;
  assert(ps->state == PS_DEAD);

  ps->name = name;
  ps->prio = prio;
  ps->parent = getpid();
  ps->ssize = ssize + 2 * sizeof(int32_t);
  ps->ssize += ps->ssize % sizeof(int32_t);
  ps->user_stack = user_stack_alloc(ps->ssize);
  if (ps->user_stack == NULL) return -1;

  const unsigned long user_stack_size = ps->ssize / sizeof(int32_t);
  ps->user_stack[user_stack_size - 2] = (int32_t)&PROC_end;
  ps->user_stack[user_stack_size - 1] = (int32_t)arg;
  ps->kernel_stack[NBSTACK - 6] = (int32_t)&JMP_usermode;
  ps->kernel_stack[NBSTACK - 5] = (int32_t)pt_func;
  ps->kernel_stack[NBSTACK - 4] = USER_CS;
  ps->kernel_stack[NBSTACK - 3] = 0x202;
  ps->kernel_stack[NBSTACK - 2] = (int32_t)&ps->user_stack[user_stack_size - 2];
  ps->kernel_stack[NBSTACK - 1] = USER_DS;
  ps->registers[1] = (int32_t)&ps->kernel_stack[NBSTACK - 6];
  push_runnable(ps);
  return ps->pid;
}
int start_user(int (*pt_func)(void*), unsigned long ssize, int prio,
          const char* name, void* arg) {
  const int pid = start_user_background(pt_func, ssize, prio, name, arg);
  fix_scheduler();
  return pid;
}

void exit(int retval) {
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
    assert(IS_VALID_PID(child_pid) && processes[child_pid].state == PS_ZOMBIE);
    child = &processes[child_pid];
  }
  if (retvalp) *retvalp = child->state_attr.retval;
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

  switch (ps->state)
  {
  case PS_ASLEEP:
    assert(asleep_process_head != NULL);
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
    break;

  case PS_WAIT_QUEUE_EMPTY:
    queue_remove_empty_process(ps);
    break;

  case PS_WAIT_QUEUE_FULL:
    queue_remove_full_process(ps);
    break;

  default:
    break;
  }

  remove_runnable(ps);
  if (ps->parent == NOPID) {
    push_dead(ps);
  } else {
    ps->state = PS_ZOMBIE;
    ps->state_attr.retval = retval;
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

void fix_scheduler() {
  if (active_process->state != PS_RUNNING ||
  (runnable_process_head != NULL && active_process->prio < runnable_process_head->prio))
    tick_scheduler();
}
/** Change running process */
void tick_scheduler() {
  {  // Wake up processes on PS_ASLEEP
    unsigned long time = current_clock();
    while (asleep_process_head != NULL && time >= asleep_process_head->state_attr.asleep.clock) {
      assert(asleep_process_head->state == PS_ASLEEP);
      struct process_t* const awake = asleep_process_head;
      asleep_process_head = asleep_process_head->state_attr.asleep.next;
      push_runnable(awake);
    }
  }

  // Active process is stopped or an other process with valid priority is runnable
  if (active_process->state != PS_RUNNING || (runnable_process_head != NULL && runnable_process_head->prio >= active_process->prio)) {
    struct process_t* prev_process = active_process;
    active_process = runnable_process_head;
    // Pop runnable
    runnable_process_head = runnable_process_head->state_attr.next_runnable;

    if (prev_process->state == PS_RUNNING) {
      push_runnable(prev_process);
    }
    active_process->state = PS_RUNNING;
    CTX_swap(prev_process->registers, active_process->registers);
  }
}
