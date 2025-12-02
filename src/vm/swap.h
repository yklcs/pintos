#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "stdbool.h"
#include "vm/addr.h"
#include "vm/page.h"
#include <stddef.h>

typedef size_t swap_slot;

void swap_init (void);
bool swap_in (swap_slot slot, vm_kpage kpage);
bool swap_out (struct page *page);

#endif /* vm/swap.h */
