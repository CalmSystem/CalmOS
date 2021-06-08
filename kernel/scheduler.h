#ifndef SCHEDULER_H_
#define SCHEDULER_H_

void setup_scheduler();
void idle();

int getpid(void);
const char* getpname(int pid);

#define NBPROC 30
#define NBSTACK 512

#endif /*SCHEDULER_H_*/
