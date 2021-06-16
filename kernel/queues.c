#include "queues.h"

#include "mem.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"

struct queue_t {
  int front, rear, size;
  int capacity;
  int *messages;
  struct process_t *empty_process;
  struct process_t *full_process;
};

struct queue_t queues[NBQUEUE] = {0};

int is_queue_full(struct queue_t *queue) {
  return queue->size == queue->capacity;
}
int is_queue_empty(struct queue_t *queue) { return queue->size == 0; }
int is_queue_free(struct queue_t *queue) { return queue->capacity == 0; }

#define VALID_FID(fid) \
if (fid < 0 || fid >= NBQUEUE || is_queue_free(&queues[fid])) return -1;

/** Add message in queue */
void push_message(struct queue_t *queue, int val) {
  assert(!is_queue_full(queue));
  queue->rear = (queue->rear + 1) % queue->capacity;
  queue->messages[queue->rear] = val;
  queue->size++;
}
/** Extract message from queue */
int pop_message(struct queue_t *queue) {
  assert(!is_queue_empty(queue));
  int val = queue->messages[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  queue->size--;
  return val;
}

/** Add process to waiting queue by priority */
void push_waiting_process(struct process_t **head, struct process_t *ps) {
  assert(ps->state == PS_WAIT_QUEUE_EMPTY || ps->state == PS_WAIT_QUEUE_FULL);
  if (*head == NULL || ps->prio > (*head)->prio) {
    ps->state_attr.wait_queue.next = *head;
    *head = ps;
  } else {
    struct process_t *cur = *head;
    while (cur->state_attr.wait_queue.next != NULL &&
           ps->prio <= cur->state_attr.wait_queue.next->prio) {
      cur = cur->state_attr.wait_queue.next;
    }
    if (cur->state_attr.wait_queue.next == NULL) {
      ps->state_attr.wait_queue.next = NULL;
    } else {
      ps->state_attr.wait_queue.next =
          cur->state_attr.wait_queue.next;
    }
    cur->state_attr.wait_queue.next = ps;
  }
}
/** Remove waiting process from queue */
void pop_waiting_process(struct process_t **head, struct process_t *ps) {
  assert(ps->state == PS_WAIT_QUEUE_EMPTY || ps->state == PS_WAIT_QUEUE_FULL);
  assert(*head != NULL);
  if (*head == ps) {
    *head = ps->state_attr.wait_queue.next;
  } else {
    struct process_t *cur = *head;
    while (cur->state_attr.wait_queue.next != NULL &&
           cur->state_attr.wait_queue.next != ps) {
      cur = cur->state_attr.wait_queue.next;
    }
    if (cur->state_attr.wait_queue.next != NULL) {
      cur->state_attr.wait_queue.next =
          ps->state_attr.wait_queue.next;
    }
  }
}

/** Wakeup first process in list */
void wakeup_first_process(struct process_t **list) {
  assert(*list != NULL);
  assert((*list)->state == PS_WAIT_QUEUE_EMPTY ||
         (*list)->state == PS_WAIT_QUEUE_FULL);
  struct process_t *ps = (*list)->state_attr.wait_queue.next;
  push_runnable(*list);
  *list = ps;
  fix_scheduler();
}
/** Wakeup all processes in list (on error) */
void wakeup_all_processes(struct process_t **list) {
  if (*list == NULL) return;
  struct process_t *cur = (*list)->state_attr.wait_queue.next;
  while (cur != NULL) {
    *(*list)->state_attr.wait_queue.retval = -1;
    push_runnable(*list);
    *list = cur;
    cur = (*list)->state_attr.wait_queue.next;
  }
  *(*list)->state_attr.wait_queue.retval = -1;
  push_runnable(*list);
  *list = NULL;
}

void queue_reorder_empty_process(struct process_t *ps) {
  assert(ps != NULL);
  assert(ps->state == PS_WAIT_QUEUE_EMPTY ||
         ps->state == PS_WAIT_QUEUE_FULL);
  struct queue_t* const queue = &queues[ps->state_attr.wait_queue.fid];
  pop_waiting_process(&queue->empty_process, ps);
  push_waiting_process(&queue->empty_process, ps);
}
void queue_reorder_full_process(struct process_t *ps) {
  assert(ps != NULL);
  assert(ps->state == PS_WAIT_QUEUE_EMPTY ||
         ps->state == PS_WAIT_QUEUE_FULL);
  struct queue_t* const queue = &queues[ps->state_attr.wait_queue.fid];
  pop_waiting_process(&queue->full_process, ps);
  push_waiting_process(&queue->full_process, ps);
}

void queue_remove_empty_process(struct process_t *ps) {
  pop_waiting_process(&queues[ps->state_attr.wait_queue.fid].empty_process, ps);
}
void queue_remove_full_process(struct process_t *ps) {
  pop_waiting_process(&queues[ps->state_attr.wait_queue.fid].full_process, ps);
}

int count_processes(struct process_t *head) {
  int count = 0;
  struct process_t *cur = head;
  while (cur != NULL) {
    cur = cur->state_attr.wait_queue.next;
    count++;
  }
  return count;
}

int pcount(int fid, int *count) {
  VALID_FID(fid);
  if (count != NULL) {
    struct queue_t *const q = &queues[fid];
    if (is_queue_empty(q)) {
      *count = -count_processes(q->empty_process);
    } else {
      *count = q->size + count_processes(q->full_process);
    }
  }
  return 0;
}

int pcreate(int count) {
  if (count <= 0 || count > 1000000000) return -3;
  for (int fid = 0; fid < NBQUEUE; fid++) {
    struct queue_t *const q = &queues[fid];
    if (!is_queue_free(q)) continue;

    q->capacity = count;
    q->messages = mem_alloc(count * sizeof(int));
    if (q->messages == NULL) return -2;
    q->rear = count - 1;
    q->front = 0;
    q->size = 0;
    return fid;
  }
  return -1;
}

int pdelete(int fid) {
  VALID_FID(fid);
  struct queue_t *const q = &queues[fid];
  q->capacity = 0;
  mem_free(q->messages, q->capacity * sizeof(int));
  wakeup_all_processes(&q->empty_process);
  wakeup_all_processes(&q->full_process);
  fix_scheduler();
  return 0;
}

int preceive(int fid, int *message) {
  VALID_FID(fid);
  struct queue_t *const q = &queues[fid];
  if (is_queue_empty(q)) {
    int retval = 0;
    struct process_t *const ps = getproc();
    remove_runnable(ps);
    ps->state = PS_WAIT_QUEUE_EMPTY;
    ps->state_attr.wait_queue.fid = fid;
    ps->state_attr.wait_queue.retval = &retval;
    ps->state_attr.wait_queue.message = message;
    push_waiting_process(&q->empty_process, ps);
    tick_scheduler();
    return retval;
  } else {
    int val = pop_message(q);
    if (message != NULL) *message = val;
    if (q->size == q->capacity - 1 && q->full_process != NULL) {
      assert(q->full_process->state_attr.wait_queue.message != NULL);
      push_message(q, *q->full_process->state_attr.wait_queue.message);
      wakeup_first_process(&q->full_process);
    }
    return 0;
  }
}

int preset(int fid) {
  VALID_FID(fid);
  struct queue_t *const q = &queues[fid];
  q->front = 0;
  q->rear = q->capacity - 1;
  q->size = 0;
  wakeup_all_processes(&q->empty_process);
  wakeup_all_processes(&q->full_process);
  fix_scheduler();
  return 0;
}

int psend(int fid, int message) {
  VALID_FID(fid);
  struct queue_t *const q = &queues[fid];
  if (is_queue_full(q)) {
    struct process_t *const ps = getproc();
    int retval = 0;
    remove_runnable(ps);
    ps->state = PS_WAIT_QUEUE_FULL;
    ps->state_attr.wait_queue.fid = fid;
    ps->state_attr.wait_queue.retval = &retval;
    ps->state_attr.wait_queue.message = &message;
    push_waiting_process(&q->full_process, ps);
    tick_scheduler();
    return retval;
  } else if (is_queue_empty(q) && q->empty_process != NULL) {
    if (q->empty_process->state_attr.wait_queue.message != NULL)
      *q->empty_process->state_attr.wait_queue.message = message;
    wakeup_first_process(&q->empty_process);
    return 0;
  } else {
    push_message(q, message);
    return 0;
  }
}