#include <stdint.h>
#include <stdio.h>
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/frame.h"

/* Number of user pages that fit in physical memory. */
extern size_t user_pages;

/* Global frame table structure. */
static struct
{
  struct frame *frames;
  struct lock lock;
} frame_table;

struct frame *
frame_find (vm_kpage kpage)
{
  void *ppage = (void *)(vtop (kpage));
  return frame_table.frames + pg_no (ppage);
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
    return NULL;
  f->page = NULL;
  f->owner = NULL;

  lock_release (&frame_table.lock);

  palloc_free_page (kpage);
}

void
frame_table_init (void)
{
  frame_table.frames = calloc (user_pages, sizeof (struct frame));
  for (uintptr_t i = 0; i < user_pages; i++)
    {
      *(vm_kpage *)&frame_table.frames[i].kpage = (void *)(i * PGSIZE);
      frame_table.frames[i].page = NULL;
      frame_table.frames[i].owner = NULL;
    }
  lock_init (&frame_table.lock);
}
