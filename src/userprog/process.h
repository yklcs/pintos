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
 * Allocated in process_execute, freed in process_wait.
 */
struct process
{
  pid_t pid;

  char *argv[MAX_ARGC]; /* Parsed arguments (with count limit) */
  int argc;             /* Length of argv */
  char *argstrs;        /* Page for strings contained in argv */

  int exit_code;
  struct semaphore exited;

  struct semaphore loaded;
  bool load_success;

  struct list fds;
  int num_fds;

  struct list_elem elem;
};

struct fd
{
  struct file *file;
  int num;
  struct list_elem elem;
};

#endif /* userprog/process.h */
