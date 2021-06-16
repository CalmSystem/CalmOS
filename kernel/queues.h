#ifndef QUEUES_H_
#define QUEUES_H_

#include "scheduler.h"

#define NBQUEUE 300

/** Writes in "count" parameter : 
        - Number of messages in queue fid + number of bloqued processes on full queue
        - Opposite of the number of bloqued processes on empty queue (negative number)
    Returns 0 or negative if invalid fid */
int pcount(int fid, int *count);

/** Allocates a queue with "count" capacity 
    Returns the created queue's id 
        or a negative number if count negative or nul or no more available queue */
int pcreate(int count);

/** Destroys the fid queue
    Returns NULL or negative if invalid fid */
int pdelete(int fid);

/** Puts the first message of the queue fid in "message" 
        and pop it off the queue (manage processes if empty queue, etc)
    Returns NULL or negative if invalid fid */
int preceive(int fid, int *message);

/** Empties the queue fid
    Returns NULL or negative if invalid fid */
int preset(int fid);

/** Puts "message" in the queue fid (manage processes if empty queue, etc) 
    Returns NULL or negative if invalid fid */
int psend(int fid, int message);

/** Update queue empty waiting list order after priority change */
void queue_reorder_empty_process(struct process_t *process);
/** Update queue full waiting list order after priority change */
void queue_reorder_full_process(struct process_t *process);

/** Remove process from queue empty waiting list */
void queue_remove_empty_process(struct process_t *process);
/** Remove process from full empty waiting list */
void queue_remove_full_process(struct process_t *process);

#endif /*QUEUES_H_*/