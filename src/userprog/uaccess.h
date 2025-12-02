#ifndef USERPROG_UACCESS_H
#define USERPROG_UACCESS_H

#include <stddef.h>
#include <stdint.h>
#include <user/syscall.h>
#include <stdbool.h>

#define UACCESS_ERROR -1

void get_user_byte (uint8_t *dst, const uint8_t *uaddr);
void put_user_byte (uint8_t *uaddr, uint8_t byte);

#define get_user(x, uaddr)                                                    \
  {                                                                           \
    for (int _i = 0; _i < sizeof ((x)); _i++)                                 \
      get_user_byte (&((uint8_t *)&(x))[_i], &((uint8_t *)uaddr)[_i]);        \
  }

#define put_user(uaddr, value)                                                \
  {                                                                           \
    typeof (value) x = value;                                                 \
    for (int _i = 0; _i < sizeof ((x)); _i++)                                 \
      put_user_byte (&((uint8_t *)uaddr)[_i], ((uint8_t *)&x)[_i]);           \
  }

void copy_from_user (void *kdst, const void *usrc, size_t size);
void copy_to_user (void *udst, const void *ksrc, size_t size);

size_t strnlen_user (const char *uaddr, size_t maxlen);

#endif /* userprog/uaccess.h */
