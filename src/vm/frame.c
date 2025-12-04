#include <stdint.h>
#include <stdio.h>
#include "filesys/file.h"
#include "stddef.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/addr.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "vm/frame.h"

/* Global frame table structure. */
struct
{
  struct frame *frames;
  int len;
  struct lock lock;

  int clock_cursor;
} frame_table;

vm_kpage choose_victim (void);

struct frame *
frame_find (void *kaddr)
{
  ASSERT (lock_held_by_current_thread (&frame_table.lock));

  void *off = kaddr - (uintptr_t)(pool_base (true));
  unsigned idx = pg_no (off);
  ASSERT (idx < frame_table.len);

  return frame_table.frames + idx;
}

vm_kpage
frame_alloc (struct page *page)
{
  void *kpage = palloc_get_page (PAL_USER);

  struct frame *f;

  if (kpage == NULL)
    {
      kpage = choose_victim ();
      if (!frame_evict (kpage))
        {
          printf ("frame_alloc: eviction failure for page 0x%x\n", kpage);
          return NULL;
        }
    }

  frame_table_lock_acquire ();

  f = frame_find (kpage);
  if (f == NULL)
    {
      printf ("frame_alloc: frame not found for page 0x%x\n", kpage);
      frame_table_lock_release ();
      return NULL;
    }

  f->page = page;
  f->owner = thread_current ();
  f->pinned = true;

  frame_table_lock_release ();

  return kpage;
}

bool
frame_free (vm_kpage kpage)
{
  struct frame *f;

  frame_table_lock_acquire ();

  f = frame_find (kpage);
  if (f == NULL)
    {
      printf ("frame_free: could not find frame 0x%x to free \n", kpage);
      frame_table_lock_release ();
      return false;
    }
  f->page = NULL;
  f->owner = NULL;
  f->pinned = false;

  frame_table_lock_release ();

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
      *(vm_kpage *)&frame->kpage
          = (void *)((uintptr_t)pool_base (true) + i * PGSIZE);
      frame->page = NULL;
      frame->owner = NULL;
      frame->pinned = false;
    }

  frame_table.clock_cursor = 0;
  lock_init (&frame_table.lock);
}

void
frame_process_cleanup (struct thread *t)
{
  int i;
  struct frame *frame;

  frame_table_lock_acquire ();
  for (i = 0; i < frame_table.len; i++)
    {
      frame = &frame_table.frames[i];
      if (frame->owner == t)
        {
          pagedir_clear_page (t->pagedir, frame->page->upage);
          frame->page = NULL;
          frame->owner = NULL;
          frame->pinned = false;
        }
    }
  frame_table_lock_release ();
}

bool
frame_evict (vm_kpage kpage)
{
  struct thread *t = thread_current ();
  struct frame *frame;
  struct page *page;
  bool fs_lock_held_ = fs_lock_held ();

  frame_table_lock_acquire ();

  frame = frame_find (kpage);
  if (frame == NULL)
    {
      printf ("frame_evict: frame to evict 0x%x not found\n", kpage);
      frame_table_lock_release ();
      return false;
    }

  if (frame->owner == NULL)
    {
      frame_table_lock_release ();
      return true;
    }

  page = frame->page;
  ASSERT (page->frame == frame);

  switch (page->type)
    {
    case VM_PAGE_ANON:
      if (!swap_out (page))
        {
          printf ("frame_evict: failed to swap out 0x%x\n", page->upage);
          frame_table_lock_release ();
          return false;
        }
      break;
    case VM_PAGE_FILE:
      if (pagedir_is_dirty (frame->owner->pagedir, page->upage))
        {
          page->type = VM_PAGE_ANON;

          if (page->finfo.mmap)
            {
              page->type = VM_PAGE_FILE;

              if (!fs_lock_held_)
                fs_lock_acquire ();
              file_write_at (page->finfo.file, kpage, page->finfo.read_bytes,
                             page->finfo.ofs);
              if (!fs_lock_held_)
                fs_lock_release ();

              pagedir_clear_page (frame->owner->pagedir, page->upage);
              page->loc = VM_LOC_FILE;
              page->frame = NULL;
            }
          else if (!swap_out (page))
            {
              printf ("frame_evict: failed to swap out 0x%x failed\n",
                      page->upage);
              frame_table_lock_release ();
              return false;
            }
          break;
        }

      pagedir_clear_page (frame->owner->pagedir, page->upage);
      page->loc = VM_LOC_FILE;
      page->frame = NULL;
      break;
    }

  frame->owner = NULL;
  frame->page = NULL;

  frame_table_lock_release ();

  return true;
}

vm_kpage
choose_victim ()
{
  struct frame *f;
  struct frame *victim;
  bool accessed;

  frame_table_lock_acquire ();
  while (1)
    {
      frame_table.clock_cursor
          = (frame_table.clock_cursor + 1) % frame_table.len;
      f = frame_table.frames + frame_table.clock_cursor;

      if (f->pinned)
        continue;

      if (f->owner == NULL)
        {
          victim = f;
          f->pinned = true;
          frame_table_lock_release ();
          return victim->kpage;
        }

      accessed = pagedir_is_accessed (f->owner->pagedir, f->page->upage);
      if (!accessed)
        {
          victim = frame_table.frames + frame_table.clock_cursor;
          f->pinned = true;
          frame_table_lock_release ();
          return victim->kpage;
        }
      pagedir_set_accessed (f->owner->pagedir, f->page->upage, false);
    }
}

void
frame_table_lock_acquire (void)
{
  lock_acquire (&frame_table.lock);
}

void
frame_table_lock_release (void)
{
  lock_release (&frame_table.lock);
}
