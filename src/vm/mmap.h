#ifndef VM_MMAP_H
#define VM_MMAP_H

#include <kernel/list.h>
#include "vm/addr.h"

typedef int mapid_t;

struct mmap
{
  mapid_t id;
  struct file *file;
  vm_upage upage;
  size_t size;
  struct list_elem elem;
};

struct mmaps
{
  struct list mmaps;
  size_t count;
};

mapid_t vm_mmap (struct file *file, vm_upage upage);
bool vm_munmap (mapid_t mapid);
void mmap_process_cleanup (struct thread *t);

#endif /* vm/mmap.h */
