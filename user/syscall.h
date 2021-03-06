#ifndef __SYSCALL_H__
#define __SYSCALL_H__
#include "system.h"
#include "file.h"

void console_putbytes(const char *str, int size); // 0

void cons_write(const char *str, unsigned long size);  // 10
int cons_read(void);                                   // 11
void cons_echo(int on);                                // 12
/** Read char until \r char. Unread chars are keeped in an internal buffer */
unsigned long cons_readline(char *string, unsigned long length); // 13
/** Beep buzzer. delay in second */
void beep(int freq, float delay); // 14

int getpid(void);                   // 20
int waitpid(int pid, int *retval);  // 21
/** Get N firsts processes status. Returns total processes count */
int processes_status(struct process_status_t *status, int count);  // 22

int chprio(int pid, int newprio);  // 30
int getprio(int pid);              // 31

int pcount(int fid, int *count);      // 40
int pcreate(int count);               // 41
int pdelete(int fid);                 // 42
int preceive(int fid, int *message);  // 43
int preset(int fid);                  // 44
int psend(int fid, int message);      // 45
/** Get N firsts queues status. Returns total queues count */
int queues_status(struct queue_status_t *status, int count);  // 46

void clock_settings(unsigned long *quartz, unsigned long *ticks);  // 50
unsigned long current_clock(void);                                 // 51
void wait_clock(unsigned long wakeup);                             // 52

int start(int (*ptfunc)(void *), unsigned long ssize, int prio,
          const char *name, void *arg);  // 60
int kill(int pid);                       // 61
void exit(int retval);                   // 62
/** Restart whole system */
void reboot();                           // 63

/** Get top level folder */
DIR fs_root();                                                              // 70
/** Get file list in directory.
 * Return error or number of files.
 * Takes an array of file_t */
int fs_list(const DIR dir, FILE *files, size_t nfiles, size_t offset);      // 71
/** Get full filename. len does not include \0 */
void fs_file_name(const FILE *f, char *name, size_t len);                   // 72
/** Read file part. Return error or readded size */
int fs_read(void *dst, const FILE *f, size_t offset, size_t len);           // 73
/** Write file part. Return error or written size */
int fs_write(const FILE *f, size_t offset, const void *src, size_t len);    //74

#endif