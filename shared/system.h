#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define CONSOLE_COL 80
#define CONSOLE_LIG 25

#define CONSOLE_BLACK 0
#define CONSOLE_BLUE 1
#define CONSOLE_GREEN 2
#define CONSOLE_CYAN 3
#define CONSOLE_RED 4
#define CONSOLE_MAGENTA 5
#define CONSOLE_BROWN 6
#define CONSOLE_LIGHT_GREY 7
#define CONSOLE_GREY 8
#define CONSOLE_LIGHT_BLUE 9
#define CONSOLE_LIGHT_GREEN 10
#define CONSOLE_LIGHT_CYAN 11
#define CONSOLE_LIGHT_RED 12
#define CONSOLE_LIGHT_MAGENTA 13
#define CONSOLE_YELLOW 14
#define CONSOLE_WHITE 15

#define NOPID -1
#define NBPROC 30
#define NBQUEUE 300


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
  /** Waiting on an empty queue */
  PS_WAIT_QUEUE_EMPTY,
  /** Waiting on a full queue */
  PS_WAIT_QUEUE_FULL
};
struct process_status_t {
  int pid;
  /** Parent process pid or NOPID */
  int parent;
  char name[20];
  int prio;
  enum process_state_t state;
  /** user_stack size in bytes */
  unsigned long ssize;
};

struct queue_status_t {
  int fid;
  int capacity;
  int count;
};

#endif
