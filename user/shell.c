#include "syscall.h"
#include "stdio.h"
#include "logo.h"
#include "string.h"
#include "shell.h"
#include "mem.h"
#include "play.h"

int test_proc(void *arg);

static struct {
	const char *name;
	void (*f) (void);
  const char* desc;
} keywords[] = {
  {"ps", ps, "Display processes in the system"},
  {"clear", clear, "Clear the terminal"},
  {"uptime", uptime, "Display how long the system has been up"},
  {"test", test, "Launch the test interface"},
  {"sys_info", sys_info, "Display some system information"},
  {"reboot", reboot, "Reboot the system"},
  {"help", help, "Display this help screen"},
  {"logo", logo, "Display the logo"},
  {"beep", _beep, "Play a short beep"},
  {"exit", _exit, "Close this shell"},
  {0, 0, 0}
};
static struct {
	const char *name;
	void (*f) (const char*);
  const char* desc;
} commands[] = {
  {"echo", echo, "Toggle echo of the terminal"},
  {"cd", cd, "Change current directory"},
  {"ls", ls, "List files in directory"},
  {"cat", cat, "Print file content"},
  {"play", play, "Play a music beep file"},
  {0, 0, 0}
};

void shell() {
  const char NONE = '\0';
  while (1) {
    char buffer[CONSOLE_COL] = {0};
    printf("> ");
    cons_readline(buffer, CONSOLE_COL);
    char* firstWord = strchr(buffer, ' ');
    if (firstWord) *firstWord = '\0';
  
    for (int i = 0; keywords[i].name; i++) {
      if (strcmp(keywords[i].name, buffer) == 0) {
        keywords[i].f();
        break;
      }
    }
    for (int i = 0; commands[i].name; i++) {
      if (strcmp(commands[i].name, buffer) == 0) {
        commands[i].f(firstWord ? firstWord + 1 : &NONE);
        break;
      }
    }
  }
}
int shell_proc(void * arg) {
  (void)arg;
  shell();
  return 0;
}

const char* PROCESS_STATE_NAMES[] = {
  "dead",
  "zombie",
  "runnable",
  "running",
  "asleep",
  "wait child",
  "wait queue empty",
  "wait queue full"
};
void ps() {
  struct process_status_t status[20];
  const int nproc = processes_status(status, 20);
  printf("PID\tNAME\t\tSTATE\t\tPRIO\tPARENT\tSSIZE\n");
  for (int i = 0; i < nproc; i++) {
    struct process_status_t* const ps = &status[i];
    printf("%d\t%-15s\t%-15s\t%d\t%d\t%lu\n", ps->pid, ps->name,
      PROCESS_STATE_NAMES[ps->state-PS_DEAD],
      ps->prio, ps->parent, ps->ssize);
  }
}
void qs() {
  struct queue_status_t status[20];
  const int nq = queues_status(status, 20);
  printf("FID\tCAPACITY\tCOUNT\n");
  for (int i = 0; i < nq; i++) {
    struct queue_status_t* const q = &status[i];
    printf("%d\t%-15d\t%d\n", q->fid, q->capacity, q->count);
  }
}

static struct {
	const char *name;
	int val;
} echo_args[] = {
  {"0", 0},
  {"1", 1},
  {"on", 1},
  {"off", 0},
  {0, 0}
};
void echo(const char* arg) {
  for (int i = 0; echo_args[i].name; i++) {
    if (strcmp(arg, echo_args[i].name) == 0) {
      cons_echo(echo_args[i].val);
      return;
    }
  }
  printf("Invalid arg\n");
}

void clear() { console_putbytes("\f", 1); }
void uptime() {
  unsigned long quartz;
  unsigned long ticks;
  clock_settings(&quartz, &ticks);
  const unsigned long seconds = current_clock() / (quartz / ticks);
  printf("%02ld:%02ld:%02ld\n", seconds / (60 * 60), (seconds / 60) % 60, seconds % 60);
}
void test() {
  int pid = start(test_proc, 4000, 128, "test", NULL);
  waitpid(pid, NULL);
}
void sys_info() {
  unsigned long quartz;
  unsigned long ticks;
  printf("CalmOS\n2021 - PILLEYRE Alexandre, BOIS Clement, GARRIGUES Enki\n\n");
  clock_settings(&quartz, &ticks);
  printf("Quartz: %ld, Ticks: %ld\n", quartz, ticks);
  printf("\n");
  ps();
  printf("\n");
  qs();
}
void _exit() { exit(0); }
void help() {
  for (int i = 0; keywords[i].name; i++) {
    printf("  %-14s\t%s\n", keywords[i].name, keywords[i].desc);
  }
  for(int i = 0; commands[i].name; i++) {
    printf("  %-14s\t%s\n", commands[i].name, commands[i].desc);
  }
}
void logo() { printf(CALMOS_LOGO); }
void _beep() { beep(1000, .1f); }

DIR pwd = {0};
int find_file(FILE* f, const char* name) {
  for (size_t i = 0;; i++) {
    if (fs_list(pwd, f, 1, i) <= 0) return 0;
    if (strcmp(name, f->name) == 0) return 1;
  }
}
void ls(const char* path) {
  FILE files[20];
  DIR root = pwd;
  if (path && *path != '\0') {
    if (find_file(&files[0], path) && (files[0].attribs & FILE_DIRECTORY)) {
      root.clusterIndex = files[0].clusterIndex;
    } else {
      cons_write("Directory not found\n", 20);
    }
  }

  int nfiles = fs_list(root, &files[0], 20, 0);
  for (int i = 0; i < nfiles; i++) {
    struct fat_datetime_t d = files[i].modifiedAt;
    printf("%c %8d \t%02d/%02d/%04d %02d:%02d \t%s\n",
           files[i].attribs & FILE_DIRECTORY ? 'd' : '-', files[i].size,
           d.date.day, d.date.month, d.date.year + FAT_YEAR_OFFSET, d.time.hour,
           d.time.minutes, files[i].name);
  }
}
void cat(const char* path) {
  FILE f;
  if (path && *path != '\0' && find_file(&f, path) && !(f.attribs & FILE_DIRECTORY)) {
    char buffer[CONSOLE_COL*CONSOLE_LIG];
    for (size_t offset = 0; offset < f.size; offset += CONSOLE_COL*CONSOLE_LIG) {
      memset(buffer, 0, CONSOLE_COL*CONSOLE_LIG);
      int size = fs_read(buffer, &f, offset, CONSOLE_COL*CONSOLE_LIG);
      if (size <= 0) break;
      cons_write(buffer, size);
    }
  } else {
    cons_write("File not found\n", 15);
  }
}
void cd(const char* path) {
  FILE f;
  if (path && *path != '\0' && find_file(&f, path) && (f.attribs & FILE_DIRECTORY)) {
    pwd.clusterIndex = f.clusterIndex;
  } else {
    cons_write("Directory not found\n", 20);
  }
}

void play(const char* path) {
  FILE f;
  if (path && *path != '\0' && find_file(&f, path) && !(f.attribs & FILE_DIRECTORY)) {
    char* buffer = mem_alloc(f.size);
    fs_read(buffer, &f, 0, f.size);
    for (char* eol = buffer-1; eol && eol < buffer + f.size; eol = strchr(eol+1, '\n')) {
      decode_music_line(eol+1);
    }
    mem_free(buffer, f.size);
  } else {
    cons_write("File not found\n", 15);
  }
}