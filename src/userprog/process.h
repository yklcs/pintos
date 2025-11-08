#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include <user/syscall.h>
#include "threads/thread.h"
#include "threads/synch.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#define MAX_ARGC 128

/*
 * Describes a process.
 * The lifetime is independent from the execution thread.
 * Allocated in process_execute, freed in process_exit.
 */
struct process
{
  pid_t pid;

  char *argv[MAX_ARGC]; /* Parsed arguments (with count limit) */
  int argc;             /* Length of argv */
  char *argbuf;         /* Buffer for argv, separately allocated */

  int exit_code;           /* Exit code */
  struct semaphore exited; /* Signaled to parent when exited */
  struct semaphore reaped; /* Signaled from parent when reaped */

  struct semaphore loaded; /* Signaled to parent when loaded */
  bool load_success;       /* Load success */
  struct file *executable; /* File process was loaded from */

  struct list fds; /* File descriptor list */
  int num_fds;     /* Number of all file descriptors (monotonic) */

  struct list_elem elem; /* Link for children of struct thread */
};

/* Associates files and file descriptors.
 * Allocated in sys_open, freed in sys_close or process_exit.
 */
struct fd
{
  struct file *file;
  int num;
  struct list_elem elem;
};

#endif /* userprog/process.h */
