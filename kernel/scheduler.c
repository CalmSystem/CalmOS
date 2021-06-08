#include "stdio.h"
#include "cpu.h"
#include "stdint.h"
#include "scheduler.h"

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
  const char* name;
  enum process_state_t state;
  int32_t registers[5];
  int32_t stack[NBSTACK];
};
struct process_t processes[NBPROC] = {0};
struct process_t* active_process;

extern void ctx_sw(int* save, int* restore);

void ordonnance();

void idle(void) {
  for (;;) {
      printf("[%s] pid = %i\n", getpname(getpid()), getpid());
      for (int32_t i = 0; i < 100000000; i++);
      ordonnance();
  }
}
void proc1(void) {
  for (;;) {
      printf("[%s] pid = %i\n", getpname(getpid()), getpid());
      for (int32_t i = 0; i < 100000000; i++);
      ordonnance(); 
  }
}

int getpid() { return active_process->pid; }
const char* getpname(int pid) {
  if (pid < 0 || pid >= NBPROC) return "";
  return processes[pid].name;
}

int add_process(const char* name, void (*entry_point)(void))
{
  for (int i = 0; i < NBPROC; i++) {
    if (processes[i].state != PS_DEAD) continue;

    processes[i].name = name;
    processes[i].state = PS_RUNNABLE;
    processes[i].stack[NBSTACK - 1] = (int32_t)entry_point;
    processes[i].registers[1] = (int32_t)&processes[i].stack[NBSTACK-1];
    return i;
  }
  return -1;
}

void setup_scheduler()
{
  for (int i = 0; i < NBPROC; i++) {
    processes[i].pid = i;
    processes[i].state = PS_DEAD;
  }
  processes[0].pid = 0;
  processes[0].name = "idle";
  processes[0].state = PS_RUNNING;
  active_process = &processes[0];
  add_process("proc1", proc1);
}

void ordonnance()
{
  struct process_t* prev_process = active_process;
  //TODO: smarter scheduler
  active_process = &processes[!active_process->pid];

  prev_process->state = PS_RUNNABLE;
  active_process->state = PS_RUNNING;
  ctx_sw(prev_process->registers, active_process->registers);
}