#include "cpu.h"
#include "stdint.h"
#include "stdbool.h"
#include "scheduler.h"
#include "interrupt.h"
#include "string.h"

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
    int* child;
    /** Next runnable process */
    struct process_t* next_runnable;
  } state_attr;
  int32_t registers[5];
  int32_t stack[NBSTACK];
};
struct process_t processes[NBPROC] = {0};
struct process_t* active_process;
struct process_t* runnable_process_head = NULL;

struct process_list_node_t
{
  struct process_t* val;
  struct process_list_node_t* next;
};
struct process_list_t
{
  struct process_list_node_t buffer[NBPROC];
  struct process_list_node_t* head;
};
/** Processes wait list for clock stored by time */
struct process_list_t wait_clock_processes = {0};

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

  for (int i = 0; i < NBPROC; i++) {
    if (processes[i].state != PS_DEAD) continue;

    processes[i].name = name;
    processes[i].prio = prio;
    processes[i].parent = getpid();
    processes[i].stack[NBSTACK - 3] = (int32_t)pt_func;
    processes[i].stack[NBSTACK - 2] = (int32_t)&process_end;
    processes[i].stack[NBSTACK - 1] = (int32_t)arg;
    processes[i].registers[1] = (int32_t)&processes[i].stack[NBSTACK - 3];
    push_runnable(&processes[i]);
    return i;
  }
  return NOPID;
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
  ps->state_attr.sleep = clock;

  // Add to wait clock queue
  struct process_list_node_t* const new_node = &wait_clock_processes.buffer[pid];
  new_node->val = ps;

  if (wait_clock_processes.head == NULL || clock <= wait_clock_processes.head->val->state_attr.sleep) {
    new_node->next = wait_clock_processes.head;
    wait_clock_processes.head = new_node;
  } else {
    struct process_list_node_t* current = wait_clock_processes.head;
    while (current->next != NULL && clock > current->next->val->state_attr.sleep) {
      current = current->next;
    }
    new_node->next = current->next;
    current->next = new_node;
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
  child->state = PS_DEAD;
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
  }
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
      child->state = PS_DEAD;
    } else {
      child->parent = NOPID;
    }
  }

  if (ps->state == PS_ASLEEP && wait_clock_processes.head != NULL) {
    // Remove from wait clock queue
    if (wait_clock_processes.head->val == ps) {
      wait_clock_processes.head = wait_clock_processes.head->next;
    } else {
      struct process_list_node_t* node = wait_clock_processes.head;
      while (node->next != NULL) {
        if (node->next->val == ps) {
          node->next = node->next->next;
          break;
        }
        node = node->next;
      }
    }
  }

  remove_runnable(ps);
  if (ps->parent == NOPID) {
    ps->state = PS_DEAD;
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
  {  // Wake up processes on PS_WAIT_CLOCK
    unsigned long time = current_clock();
    while (wait_clock_processes.head != NULL &&
           time >= wait_clock_processes.head->val->state_attr.sleep) {
      // assert(wait_clock_processes.head->val->state == PS_WAIT_CLOCK);
      push_runnable(wait_clock_processes.head->val);
      wait_clock_processes.head = wait_clock_processes.head->next;
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