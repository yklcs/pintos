#include "userprog/syscall.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "list.h"
#include "stdio.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include <stddef.h>
#include <user/syscall.h>
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/uaccess.h"

static void syscall_handler (struct intr_frame *);

struct fd *find_fd (int fdnum);

/* Individual syscall handling functions */
typedef void sys_fn (struct intr_frame *);
sys_fn sys_halt NO_RETURN;
sys_fn sys_exit NO_RETURN;
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
sys_halt (struct intr_frame *if_ UNUSED)
{
  shutdown_power_off ();
}

void
sys_exit (struct intr_frame *if_)
{
  struct process *proc = thread_current ()->process;
  int exit_code; /* Arg */

  get_user (exit_code, if_->esp + sizeof (int));

  proc->exit_code = exit_code;
  thread_exit ();
}

void
sys_exec (struct intr_frame *if_)
{
  pid_t child;
  const char *cmd; /* Arg */

  get_user (cmd, if_->esp + sizeof (int));
  strnlen_user (cmd, PGSIZE); /* Performs validation */

  child = process_execute (cmd);

  if_->eax = child;
}

void
sys_wait (struct intr_frame *if_)
{
  int child_exit_code;
  pid_t child; /* Arg */

  get_user (child, if_->esp + sizeof (int));

  child_exit_code = process_wait (child);

  if_->eax = child_exit_code;
}

void
sys_create (struct intr_frame *if_)
{
  bool ok;
  char *filename; /* Arg */
  unsigned size;  /* Arg */

  get_user (filename, if_->esp + sizeof (int));
  get_user (size, if_->esp + sizeof (int) + sizeof (char *));
  strnlen_user (filename, PGSIZE); /* Performs validation */

  fs_lock_acquire ();
  ok = filesys_create (filename, size);
  fs_lock_release ();

  if_->eax = ok;
}

void
sys_remove (struct intr_frame *if_)
{
  bool ok;
  char *filename; /* Arg */

  get_user (filename, if_->esp + sizeof (int));
  strnlen_user (filename, PGSIZE); /* Performs validation */

  fs_lock_acquire ();
  ok = filesys_remove (filename);
  fs_lock_release ();

  if_->eax = ok;
}

void
sys_open (struct intr_frame *if_)
{
  struct process *proc = thread_current ()->process;
  struct fd *fd;
  char *filename; /* Arg */

  get_user (filename, if_->esp + sizeof (int));
  strnlen_user (filename, PGSIZE); /* Performs validation */

  /* Try to create a new FD */
  fd = palloc_get_page (PAL_ZERO);
  if (fd == NULL)
    {
      if_->eax = -1;
      return;
    }

  fs_lock_acquire ();
  fd->file = filesys_open (filename);
  fs_lock_release ();

  /* Failed to open */
  if (fd->file == NULL)
    {
      palloc_free_page (fd);
      if_->eax = -1;
      return;
    }

  fd->num = proc->num_fds++;
  list_push_back (&proc->fds, &fd->elem);

  if_->eax = fd->num;
}

void
sys_filesize (struct intr_frame *if_)
{
  struct fd *fd;
  int filesize;
  int fdnum; /* Arg */

  get_user (fdnum, if_->esp + sizeof (int));

  /* Find FD */
  if ((fd = find_fd (fdnum)) == NULL)
    {
      if_->eax = -1;
      return;
    }

  fs_lock_acquire ();
  filesize = file_length (fd->file);
  fs_lock_release ();

  if_->eax = filesize;
}

void
sys_read (struct intr_frame *if_)
{
  struct fd *fd;
  unsigned bytes_read;
  int fdnum;     /* Arg */
  char *buf;     /* Arg */
  unsigned size; /* Arg */

  get_user (fdnum, if_->esp + sizeof (int));
  get_user (buf, if_->esp + sizeof (int) + sizeof (int));
  get_user (size, if_->esp + sizeof (int) + sizeof (int) + sizeof (void *));
  validate_uaddrs (buf, size);

  /* Read from stdin */
  if (fdnum == STDIN_FILENO)
    {
      for (bytes_read = 0; bytes_read < size; bytes_read++)
        buf[bytes_read] = input_getc ();
      if_->eax = size;
      return;
    }

  /* Find FD */
  if ((fd = find_fd (fdnum)) == NULL)
    {
      if_->eax = -1;
      return;
    }

  fs_lock_acquire ();
  bytes_read = file_read (fd->file, buf, size);
  fs_lock_release ();

  if_->eax = bytes_read;
}

void
sys_write (struct intr_frame *if_)
{
  struct fd *fd;
  unsigned bytes_written;
  int fdnum;     /* Arg */
  char *buf;     /* Arg */
  unsigned size; /* Arg */

  get_user (fdnum, if_->esp + sizeof (int));
  get_user (buf, if_->esp + sizeof (int) + sizeof (int));
  get_user (size, if_->esp + sizeof (int) + sizeof (int) + sizeof (void *));
  validate_uaddrs (buf, size);

  /* Write to stdout */
  if (fdnum == STDOUT_FILENO)
    {
      putbuf (buf, size); /* Unbuffered write */
      if_->eax = size;
      return;
    }

  /* Find FD */
  if ((fd = find_fd (fdnum)) == NULL)
    {
      if_->eax = -1;
      return;
    }

  fs_lock_acquire ();
  bytes_written = file_write (fd->file, buf, size);
  fs_lock_release ();

  if_->eax = bytes_written;
}

void
sys_seek (struct intr_frame *if_)
{
  struct fd *fd;
  int fdnum; /* Arg */
  int pos;   /* Arg */

  get_user (fdnum, if_->esp + sizeof (int));
  get_user (pos, if_->esp + sizeof (int) + sizeof (int));

  /* Find FD */
  if ((fd = find_fd (fdnum)) == NULL)
    return;

  fs_lock_acquire ();
  file_seek (fd->file, pos);
  fs_lock_release ();
}

void
sys_tell (struct intr_frame *if_)
{
  struct fd *fd;
  int fdnum; /* Arg */
  int pos;

  get_user (fdnum, if_->esp + sizeof (int));

  /* Find FD */
  if ((fd = find_fd (fdnum)) == NULL)
    {
      if_->eax = -1;
      return;
    }

  fs_lock_acquire ();
  pos = file_tell (fd->file);
  fs_lock_release ();

  if_->eax = pos;
}

void
sys_close (struct intr_frame *if_)
{
  struct fd *fd;
  int fdnum; /* Arg */

  get_user (fdnum, if_->esp + sizeof (int));

  /* Find FD */
  if ((fd = find_fd (fdnum)) == NULL)
    {
      if_->eax = -1;
      return;
    }

  fs_lock_acquire ();
  file_close (fd->file);
  fs_lock_release ();

  list_remove (&fd->elem);
  palloc_free_page (fd);
}

/* Find the given file descriptor of the current process. */
struct fd *
find_fd (int fdnum)
{
  struct process *proc = thread_current ()->process;
  struct list_elem *pos, *next;
  struct fd *fd;

  list_foreach (&proc->fds, pos, next)
  {
    fd = list_entry (pos, struct fd, elem);
    if (fd->num == fdnum)
      return fd;
  }

  return NULL;
}
