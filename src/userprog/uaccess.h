#ifndef USERPROG_UACCESS_H
#define USERPROG_UACCESS_H

#include <stddef.h>
#include <user/syscall.h>
#include <stdbool.h>

void validate_uaddr (const void *uaddr);

/* Gets value at user address uaddr and reads it into x.
   Validation is performed. */
#define get_user(x, uaddr)                                                    \
  ({                                                                          \
    validate_uaddr ((uaddr));                                                 \
    (x) = *(typeof (&(x)))(uaddr);                                            \
  })

/* Stores value at user address uaddr.
   Validation is performed. */
#define put_user(uaddr, value)                                                \
  ({                                                                          \
    validate_uaddr ((uaddr));                                                 \
    *(typeof ((0 + value)) *)(uaddr) = (value);                               \
  })

void copy_from_user (void *kdst, const void *usrc, size_t size);
void copy_to_user (void *udst, const void *ksrc, size_t size);

size_t strnlen_user (const char *uaddr, size_t maxlen);

#endif /* userprog/uaccess.h */
