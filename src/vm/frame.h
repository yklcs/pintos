#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/synch.h"
#include "vm/addr.h"

struct frame
{
  const vm_kpage kpage;
  struct thread *owner;
  struct page *page;
};

void frame_table_init (void);
struct frame *frame_find (vm_kpage kpage);
vm_kpage frame_alloc (struct page *page);
bool frame_free (vm_kpage kpage);

#endif /* vm/frame.h */
