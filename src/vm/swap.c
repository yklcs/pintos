#include <kernel/bitmap.h>
#include <stdio.h>
#include "devices/block.h"
#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/page.h"

/* Global swap space. */
static struct
{
  struct bitmap *used_map;
  struct block *device;
  struct lock lock;
  swap_slot size;
} swap;

void
swap_init (void)
{
  swap.device = block_get_role (BLOCK_SWAP);
  if (swap.device == NULL)
    printf ("swap_init: could not get swap block device\n");

  swap.size = block_size (swap.device) * BLOCK_SECTOR_SIZE / PGSIZE;
  swap.used_map = bitmap_create (swap.size);
  lock_init (&swap.lock);
}

bool
swap_in (swap_slot slot, vm_kpage kpage)
{
  block_sector_t sector;
  int i;

  lock_acquire (&swap.lock);

  if (!bitmap_test (swap.used_map, slot))
    {
      printf ("swap_in: requested slot is not used\n");
      lock_release (&swap.lock);
      return false;
    }

  for (i = 0; i < SLOTS_PER_PAGE; i++)
    {
      sector = slot * SLOTS_PER_PAGE + i;
      block_read (swap.device, sector, kpage + i * BLOCK_SECTOR_SIZE);
    }

  bitmap_reset (swap.used_map, slot);

  lock_release (&swap.lock);

  return true;
}

bool
swap_out (struct page *page)
{
  swap_slot slot;
  block_sector_t sector;
  int i;

  if (page == NULL)
    return false;
  if (page->loc != VM_LOC_MEM)
    return false;
  if (page->type == VM_PAGE_FILE && page->finfo.mmap)
    return false;

  lock_acquire (&swap.lock);

  slot = bitmap_scan_and_flip (swap.used_map, 0, 1, false);
  if (slot == BITMAP_ERROR)
    {
      printf ("swap_out: failed to allocate swap slot\n");
      lock_release (&swap.lock);
      return false;
    }

  for (i = 0; i < SLOTS_PER_PAGE; i++)
    {
      sector = slot * SLOTS_PER_PAGE + i;
      block_write (swap.device, sector,
                   page->frame->kpage + i * BLOCK_SECTOR_SIZE);
    }

  lock_release (&swap.lock);

  page->loc = VM_LOC_SWAP;
  page->swinfo.swap_slot = slot;
  pagedir_clear_page (page->frame->owner->pagedir, page->upage);
  page->frame = NULL;

  return true;
}
