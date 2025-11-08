#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/uaccess.h"

static void syscall_handler (struct intr_frame *);

/* Type of the individual syscall handling functions */
typedef void sys_fn (struct intr_frame *);

sys_fn sys_halt;
sys_fn sys_exit;
sys_fn sys_exec;
sys_fn sys_wait;
sys_fn sys_create;
sys_fn sys_remove;
sys_fn sys_open;
sys_fn sys_filesize;
sys_fn sys_read;
sys_fn sys_write;
sys_fn sys_seek;
sys_fn sys_tell;
sys_fn sys_close;

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *if_)
{
  int syscall_num;
  get_user (syscall_num, if_->esp);
  // int syscall_num = *(int *)if_->esp;

  sys_fn *sys_fns[] = {
    [SYS_HALT] = sys_halt,     [SYS_EXIT] = sys_exit,
    [SYS_EXEC] = sys_exec,     [SYS_WAIT] = sys_wait,
    [SYS_CREATE] = sys_create, [SYS_REMOVE] = sys_remove,
    [SYS_OPEN] = sys_open,     [SYS_FILESIZE] = sys_filesize,
    [SYS_READ] = sys_read,     [SYS_WRITE] = sys_write,
    [SYS_SEEK] = sys_seek,     [SYS_TELL] = sys_tell,
    [SYS_CLOSE] = sys_close,
  };

  sys_fns[syscall_num](if_);
}

void
sys_halt (struct intr_frame *if_)
{
  shutdown_power_off ();
}

void
sys_exit (struct intr_frame *if_)
{
  struct process *proc = thread_current ()->process;
  int exit_code;
  get_user (exit_code, if_->esp + sizeof (int));

  proc->exit_code = exit_code;
  thread_exit ();
}

void
sys_exec (struct intr_frame *if_)
{
}

void
sys_wait (struct intr_frame *if_)
{
}

void
sys_create (struct intr_frame *if_)
{
}

void
sys_remove (struct intr_frame *if_)
{
}

void
sys_open (struct intr_frame *if_)
{
}

void
sys_filesize (struct intr_frame *if_)
{
}

void
sys_read (struct intr_frame *if_)
{
}

void
sys_write (struct intr_frame *if_)
{
  int fd = *(int *)(if_->esp + 4);
  const char *buf = *(const char **)(if_->esp + 8);
  unsigned size = *(unsigned *)(if_->esp + 12);

  if (fd == 1)
    {
      putbuf (buf, size);
    }
}

void
sys_seek (struct intr_frame *if_)
{
}

void
sys_tell (struct intr_frame *if_)
{
}

void
sys_close (struct intr_frame *if_)
{
}
