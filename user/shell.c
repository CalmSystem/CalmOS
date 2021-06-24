#include "syscall.h"
#include "stdio.h"
#include "logo.h"
#include "string.h"
#include "shell.h"

int test_proc(void *arg);

static struct {
	const char *name;
	void (*f) (void);
  const char* desc;
} commands[] = {
  {"ps", ps, "Display processes in the system"},
  {"echo on", echo_on, "Enable echo of the terminal"},
  {"echo off", echo_off, "Disable echo of the terminal"},
  {"clear", clear, "Clear the terminal"},
  {"uptime", uptime, "Display how long the system has been up"},
  {"test", test, "Launch the test interface"},
  {"sys_info", sys_info, "Display some system information"},
  {"reboot", reboot, "Reboot the system"},
  {"help", help, "Display this help screen"},
  {"logo", logo, "Display the logo"},
  {"beep", _beep, "Play a short beep"},
  {"exit", _exit, "Close this shell"},
  {"cd", cd, "Change current directory"},
  {"ls", ls, "List files in directory"},
  {"cat", cat, "Print file content"},
  {0, 0, 0}
};

void shell() {
  while (1) {
    char buffer[CONSOLE_COL] = {0};
    printf("> ");
    cons_readline(buffer, CONSOLE_COL);
    for (int i = 0; commands[i].name; i++) {
      if (strcmp(commands[i].name, buffer) == 0) {
        commands[i].f();
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
void echo_on() { cons_echo(1); }
void echo_off() { cons_echo(0); }
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
  for(int i = 0; commands[i].name; i++) {
    printf("  %-14s\t%s\n", commands[i].name, commands[i].desc);
  }
}
void logo() { printf(CALMOS_LOGO); }
void _beep() { beep(1000, .1f); }

DIR pwd = {0};
void ls() {
  FILE files[20];
  int nfiles = fs_list(pwd, &files[0], 20, 0);
  for (int i = 0; i < nfiles; i++) {
    struct fat_datetime_t d = files[i].modifiedAt;
    printf("%c %8d \t%02d/%02d/%04d %02d:%02d \t%s\n",
           files[i].attribs & FILE_DIRECTORY ? 'd' : '-', files[i].size,
           d.date.day, d.date.month, d.date.year + FAT_YEAR_OFFSET, d.time.hour,
           d.time.minutes, files[i].name);
  }
}
int prompt_file(FILE *f) {
  char name[FILE_SHORTNAME_SIZE+1] = {0};
  cons_readline(name, FILE_SHORTNAME_SIZE);
  for (size_t i = 0;; i++) {
    if (fs_list(pwd, f, 1, i) <= 0) return 0;
    if (strcmp(name, f->name) == 0) return 1;
  }
}
void cat() {
  FILE f;
  printf("File: ");
  if (prompt_file(&f) && !(f.attribs & FILE_DIRECTORY)) {
    char buffer[CONSOLE_COL];
    for (size_t offset = 0; offset < f.size; offset += CONSOLE_COL) {
      memset(buffer, 0, CONSOLE_COL);
      int size = fs_read(buffer, &f, offset, CONSOLE_COL);
      if (size <= 0) break;
      cons_write(buffer, size);
    }
  } else {
    cons_write("File not found\n", 15);
  }
}
void cd() {
  FILE f;
  printf("Dir: ");
  if (prompt_file(&f) && (f.attribs & FILE_DIRECTORY)) {
    pwd.clusterIndex = f.clusterIndex;
  } else {
    cons_write("Directory not found\n", 20);
  }
}
