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

int chprio(int pid, int newprio);  // 30
int getprio(int pid);              // 31

int pcount(int fid, int *count);      // 40
int pcreate(int count);               // 41
int pdelete(int fid);                 // 42
int preceive(int fid, int *message);  // 43
int preset(int fid);                  // 44
int psend(int fid, int message);      // 45

void clock_settings(unsigned long *quartz, unsigned long *ticks);  // 50
unsigned long current_clock(void);                                 // 51
void wait_clock(unsigned long wakeup);                             // 52

int start(int (*ptfunc)(void *), unsigned long ssize, int prio,
          const char *name, void *arg);  // 60
int kill(int pid);                       // 61
void exit(int retval);                   // 62
