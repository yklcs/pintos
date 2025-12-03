#include "list.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include <user/syscall.h>
#include "vm/mmap.h"
#include "userprog/pagedir.h"
#include "vm/addr.h"
#include "threads/malloc.h"
#include "vm/page.h"
#include "vm/vm.h"

bool region_unused (void *uaddr, size_t size);

mapid_t
vm_mmap (struct file *file, vm_upage upage)
{
  size_t filesize;
  size_t off;
  size_t read_bytes;
  size_t zero_bytes;

  if (upage == NULL || !is_user_vaddr (upage) || pg_ofs (upage) != 0)
    return MAP_FAILED;

  fs_lock_acquire ();

  filesize = file_length (file);
  if (filesize == 0)
    {
      fs_lock_release ();
      return MAP_FAILED;
    }

  if (!region_unused (upage, filesize))
    {
      fs_lock_release ();
      return MAP_FAILED;
    }

  struct mmaps *mmaps = &thread_current ()->mmaps;
  struct mmap *mmap = malloc (sizeof (struct mmap));
  mmap->id = mmaps->count++;
  mmap->file = file_reopen (file);
  mmap->upage = upage;
  mmap->size = filesize;

  fs_lock_release ();

  list_push_back (&mmaps->mmaps, &mmap->elem);

  for (off = 0; off < mmap->size; off += PGSIZE)
    {
      read_bytes = mmap->size - off < PGSIZE ? mmap->size - off : PGSIZE;
      zero_bytes = PGSIZE - read_bytes;
      vm_map_file (upage + off, true, mmap->file, off, read_bytes, zero_bytes,
                   true);
    }

  return mmap->id;
}

bool
region_unused (void *uaddr, size_t size)
{
  void *cur;
  vm_upage upage;

  for (cur = uaddr; cur < uaddr + size; cur += PGSIZE)
    {
      if (page_find (cur))
        return false;
    }

  return true;
}

bool
vm_munmap (mapid_t mapid)
{
  struct thread *t = thread_current ();
  struct mmap *mmap = NULL;
  struct list_elem *pos, *next;
  vm_upage upage;
  struct page *page;
  size_t off;

  list_foreach (&t->mmaps.mmaps, pos, next)
  {
    mmap = list_entry (pos, struct mmap, elem);
    if (mmap->id == mapid)
      break;
  }
  if (mmap->id != mapid)
    return false;

  for (off = 0; off < mmap->size; off += PGSIZE)
    {
      upage = mmap->upage + off;
      page = page_find (upage);

      if (page->loc == VM_LOC_MEM)
        {
          if (pagedir_is_dirty (t->pagedir, upage))
            {
              fs_lock_acquire ();
              file_write_at (mmap->file, upage, page->finfo.read_bytes,
                             page->finfo.ofs);
              fs_lock_release ();
            }
          frame_free (page->frame->kpage);
          pagedir_clear_page (t->pagedir, upage);
        }

      page->loc = VM_LOC_FILE;
      page->frame = NULL;
      hash_delete (&t->page_map.pages, &page->elem);
      free (page);
    }

  list_remove (&mmap->elem);
  file_close (mmap->file);
  free (mmap);

  return true;
}

void
mmap_process_cleanup (struct thread *t)
{
  struct mmap *mmap = NULL;
  struct list_elem *pos, *next;

  list_foreach (&t->mmaps.mmaps, pos, next)
  {
    struct mmap *mmap = list_entry (pos, struct mmap, elem);
    vm_munmap (mmap->id);
  }
}