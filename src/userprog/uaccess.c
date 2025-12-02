#include "userprog/uaccess.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "vm/page.h"
#include <stddef.h>
#include <user/syscall.h>
#include <stdbool.h>
#include <stdint.h>

void
get_user_byte (uint8_t *dst, const uint8_t *uaddr)
{
  int result;

  if (!is_user_vaddr (uaddr))
    goto fail;

  asm ("movl $1f, %0;"
       "movzbl %1, %0;"
       "1:"
       : "=&a"(result)
       : "m"(*uaddr));

  if (result == UACCESS_ERROR)
    goto fail;

  *dst = (uint8_t)result;
  return;

fail:
  thread_current ()->process->exit_code = EXIT_CRITICAL;
  thread_exit ();
}

void
put_user_byte (uint8_t *uaddr, uint8_t byte)
{
  int error_code;

  if (!is_user_vaddr (uaddr))
    goto fail;

  asm ("movl $1f, %0;"
       "movb %b2, %1;"
       "1:"
       : "=&a"(error_code), "=m"(*uaddr)
       : "q"(byte));

  if (error_code == UACCESS_ERROR)
    goto fail;

  return;

fail:
  thread_current ()->process->exit_code = EXIT_CRITICAL;
  thread_exit ();
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
      get_user (c, uaddr + len);
      len++;
      if (c == '\0')
        break;
    }

  return len;
}
