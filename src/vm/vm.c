#include <string.h>
#include "vm/vm.h"
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/addr.h"
#include "vm/page.h"
#include "vm/frame.h"

/* Initialize the virtual memory system. */
void
vm_init (void)
{
  frame_table_init ();
}

bool
vm_process_init (void)
{
  struct thread *t = thread_current ();
  return page_map_init (&t->page_map);
}

void
vm_process_exit (void)
{
  struct thread *t = thread_current ();
  page_map_destroy (&t->page_map);
  frame_process_cleanup (t);
}

bool
vm_map_file (vm_upage upage, bool writable, struct file *file, off_t ofs,
             uint32_t read_bytes, uint32_t zero_bytes, bool file_writable)
{
  struct thread *t = thread_current ();
  struct page *page = malloc (sizeof (struct page));
  if (page == NULL)
    return false;

  page->upage = upage;
  page->frame = NULL;
  page->writable = writable;
  page->type = VM_PAGE_FILE;
  page->loc = VM_LOC_FILE;
  page->finfo.file = file;
  page->finfo.ofs = ofs;
  page->finfo.read_bytes = read_bytes;
  page->finfo.zero_bytes = zero_bytes;
  page->finfo.writable = file_writable;

  hash_insert (&t->page_map.pages, &page->elem);

  return true;
}

bool
vm_map_zero (vm_upage upage, bool writable)
{
  struct thread *t = thread_current ();
  struct page *page = malloc (sizeof (struct page));
  if (page == NULL)
    return false;

  page->upage = upage;
  page->frame = NULL;
  page->writable = writable;
  page->type = VM_PAGE_ANON;
  page->loc = VM_LOC_ZERO;

  hash_insert (&t->page_map.pages, &page->elem);

  return true;
}

bool
vm_load (vm_upage upage)
{
  struct thread *t = thread_current ();
  bool ok = false;

  struct page *page;
  vm_kpage kpage;

  page = page_find (upage);
  if (page == NULL)
    return false;

  kpage = frame_alloc (page->upage);
  if (kpage == NULL)
    return false;

  switch (page->loc)
    {
    case VM_LOC_ZERO:
      memset (kpage, 0, PGSIZE);
      ok = true;
      break;
    case VM_LOC_FILE:
      ok = (file_read_at (page->finfo.file, kpage, page->finfo.read_bytes,
                          page->finfo.ofs)
            == page->finfo.read_bytes);
      if (ok)
        memset (kpage + page->finfo.read_bytes, 0, page->finfo.zero_bytes);
      break;
    case VM_LOC_SWAP:
      break;
    default:
      ok = false;
    }
  if (!ok)
    {
      frame_free (kpage);
      return false;
    }

  ok = pagedir_set_page (t->pagedir, page->upage, kpage, page->writable);
  if (!ok)
    {
      frame_free (kpage);
      return false;
    }

  page->frame = frame_find (kpage);
  page->frame->page = page;
  page->frame->owner = t;
  page->loc = VM_LOC_MEM;

  return true;
}

bool
vm_fault (void *uaddr)
{
  vm_upage upage = pg_round_down (uaddr);
  // if (!is_user_vaddr (upage))
  //   return false;

  return vm_load (upage);
}

bool
vm_grow_stack (void *uaddr, void *esp)
{
  bool within_limit = uaddr >= PHYS_BASE - STACK_LIMIT;
  bool near_esp = uaddr >= esp - 32;
  vm_upage upage;

  if (!within_limit || !near_esp)
    return false;

  upage = pg_round_down (uaddr);
  vm_map_zero (upage, true);
  return vm_load (upage);
}
