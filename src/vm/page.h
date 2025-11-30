#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <kernel/hash.h>
#include "vm/addr.h"
#include "vm/frame.h"
#include "filesys/off_t.h"

struct page_map
{
  struct hash pages;
};

enum page_type
{
  VM_PAGE_ANON,
  VM_PAGE_FILE,
};

enum page_loc
{
  VM_LOC_MEM,
  VM_LOC_SWAP,
  VM_LOC_FILE,
  VM_LOC_ZERO
};

struct page
{
  vm_upage upage;
  struct frame *frame;

  bool writable;
  enum page_type type;
  enum page_loc loc;

  union
  {
    struct
    {
      struct file *file;
      off_t ofs;
      uint32_t read_bytes;
      uint32_t zero_bytes;
      bool writable;
    } finfo;

    struct
    {
      size_t swap_slot;
    } swinfo;
  };

  struct hash_elem elem;
};

bool page_map_init (struct page_map *map);
struct page *page_find (vm_upage upage);

#endif /* vm/page.h */
