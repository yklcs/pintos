#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/synch.h"
#include "vm/addr.h"

struct frame
{
  const vm_kpage kpage;
  struct thread *owner;
  struct page *page;
  bool pinned;
};

void frame_table_init (void);
struct frame *frame_find (vm_kpage kpage);
vm_kpage frame_alloc (struct page *page);
bool frame_free (vm_kpage kpage);
bool frame_evict (vm_kpage kpage);
void frame_process_cleanup (struct thread *t);

void frame_table_lock_acquire (void);
void frame_table_lock_release (void);

#endif /* vm/frame.h */
