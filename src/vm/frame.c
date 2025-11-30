#include <stdint.h>
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/frame.h"

/* Global frame table structure. */
static struct
{
  struct frame *frames;
  int len;
  struct lock lock;
} frame_table;

struct frame *
frame_find (void *kaddr)
{
  void *off = kaddr - (uintptr_t)(pool_base (true));
  return frame_table.frames + pg_no (off);
}

vm_kpage
frame_alloc (struct page *page)
{
  void *kpage = palloc_get_page (PAL_USER);
  struct frame *f;

  if (kpage == NULL)
    return NULL;

  lock_acquire (&frame_table.lock);

  f = frame_find (kpage);
  if (f == NULL)
    return NULL;
  f->page = page;
  f->owner = thread_current ();

  lock_release (&frame_table.lock);

  return kpage;
}

bool
frame_free (vm_kpage kpage)
{
  struct frame *f;

  lock_acquire (&frame_table.lock);

  f = frame_find (kpage);
  if (f == NULL)
    return false;
  f->page = NULL;
  f->owner = NULL;

  lock_release (&frame_table.lock);

  palloc_free_page (kpage);
  return true;
}

void
frame_table_init (void)
{
  int i;
  struct frame *frame;

  frame_table.len = pool_size (true);
  frame_table.frames = calloc (frame_table.len, sizeof (struct frame));
  for (i = 0; i < frame_table.len; i++)
    {
      frame = &frame_table.frames[i];
      *(vm_kpage *)&frame->kpage = (void *)(i * PGSIZE);
      frame->page = NULL;
      frame->owner = NULL;
    }
  lock_init (&frame_table.lock);
}

void
frame_process_cleanup (struct thread *t)
{
  int i;
  struct frame *frame;

  lock_acquire (&frame_table.lock);
  for (i = 0; i < frame_table.len; i++)
    {
      frame = &frame_table.frames[i];
      if (frame->owner == t)
        {
          frame->page = NULL;
          frame->owner = NULL;
        }
    }
  lock_release (&frame_table.lock);
}
