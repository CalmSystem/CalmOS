#ifndef __SHELL_H__
#define __SHELL_H__

/** Shell entry function */
void shell();
/** Calls shell() in process interface */
int shell_proc(void *);

/** Display processes in the system */
void ps();
/** Enable echo of the terminal */
void echo_on();
/** Disable echo of the terminal */
void echo_off();
/** Clear the terminal */
void clear();
/** Display how long the system has been up */
void uptime();
/** Launch the test interface */
void test();
/** Display some system information */
void sys_info();
/** Close this shell */
void _exit();
/** Display help screen */
void help();
/** Display the logo */
void logo();
/** Play a short beep */
void _beep();
/** Change current directory */
void cd();
/** List files in directory */
void ls();
/** Print file content */
void cat();

#endif
