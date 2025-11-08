#include "userprog/uaccess.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <stddef.h>
#include <user/syscall.h>
#include <stdbool.h>
#include <stdint.h>

/* Validates uaddr as a user address.
   Exits the thread if invalid.  */
void
validate_uaddr (const void *uaddr)
{
  struct thread *t = thread_current ();

  /* Basic cases */
  if (uaddr == NULL || !is_user_vaddr (uaddr))
    {
      t->process->exit_code = EXIT_CRITICAL;
      thread_exit ();
    }

  /* Check pagedir */
  if (pagedir_get_page (t->pagedir, uaddr) == NULL)
    {
      t->process->exit_code = EXIT_CRITICAL;
      thread_exit ();
    }
}

/* Copies size bytes from user address usrc to user addess udst.
   Backed by get_user for safety. */
void
copy_from_user (void *kdst, const void *usrc, size_t size)
{
  uint8_t *k_ptr = (uint8_t *)kdst;
  const uint8_t *u_ptr = (const uint8_t *)usrc;

  for (size_t i = 0; i < size; i++)
    {
      get_user (k_ptr[i], u_ptr + i);
    }
}

/* Copies size bytes from kernel address ksrc to user addess usrc.
   Backed by put_user for safety. */
void
copy_to_user (void *udst, const void *ksrc, size_t size)
{
  const uint8_t *k_ptr = (const uint8_t *)ksrc;
  uint8_t *u_ptr = (uint8_t *)udst;

  for (size_t i = 0; i < size; i++)
    {
      put_user (u_ptr + i, k_ptr[i]);
    }
}

/* Safe strnlen operation on user address. */
size_t
strnlen_user (const char *uaddr, size_t maxlen)
{
  size_t len = 0;
  char c;

  while (len < maxlen)
    {
      get_user (c, uaddr + len++);
      if (c == '\0')
        break;
    }

  return len;
}
