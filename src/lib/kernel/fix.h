#ifndef __LIB_KERNEL_FIX_H
#define __LIB_KERNEL_FIX_H

#include <stdint.h>

#define FRAC_BITS 14

/* Q(31-FRAC_BITS).FRAC_BITS fixed point type */
typedef int32_t fix_t;

#define UNIT (1 << FRAC_BITS)
#define UNIT_HALF (1 << (FRAC_BITS - 1))

/* int to fix_t */
#define inttofix(x) ((fix_t)((x) * UNIT))

/* fix_t to int, round towards zero (truncate) */
#define fixrtz(x) ((int)((x) / UNIT))

/* fix_t to int, round to nearest, ties to away */
#define fixrna(x)                                                             \
  ((int)(((x) >= 0 ? ((x) + UNIT_HALF) : ((x) - UNIT_HALF)) / UNIT))

/* Addition */
#define fixadd(x, y) ((x) + (y))

/* Subtraction */
#define fixsub(x, y) ((x) - (y))

/* Multiplication */
#define fixmul(x, y) ((fix_t)((((int64_t)(x) * (y)) / UNIT)))

/* Division */
#define fixdiv(x, y) ((fix_t)((((int64_t)(x) * UNIT) / (y))))

#endif /* lib/kernel/fix.h */
