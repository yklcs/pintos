#include "vm/page.h"
#include <kernel/hash.h>
#include <stdbool.h>
#include <stddef.h>

#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/frame.h"

hash_hash_func page_hash_hash;
hash_less_func page_hash_less;
hash_action_func page_hash_destroy;

struct page *
page_find (const void *uaddr)
{
  struct page_map *map = &thread_current ()->page_map;
  struct page page_tmp;
  struct hash_elem *e;

  page_tmp.upage = pg_round_down (uaddr);
  e = hash_find (&map->pages, &page_tmp.elem);
  if (e == NULL)
    return NULL;

  return hash_entry (e, struct page, elem);
}

bool
page_map_init (struct page_map *map)
{
  return hash_init (&map->pages, page_hash_hash, page_hash_less, NULL);
}

unsigned
page_hash_hash (const struct hash_elem *e, UNUSED void *aux)
{
  struct page *pg = hash_entry (e, struct page, elem);
  return hash_bytes (&pg->upage, sizeof (pg->upage));
}

void
page_map_destroy (struct page_map *map)
{
  hash_destroy (&map->pages, page_hash_destroy);
}

bool
page_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                UNUSED void *aux)
{
  struct page *pg_a = hash_entry (a, struct page, elem);
  struct page *pg_b = hash_entry (b, struct page, elem);

  return pg_a->upage < pg_b->upage;
}

void
page_hash_destroy (struct hash_elem *elem, UNUSED void *aux)
{
  struct page *page = hash_entry (elem, struct page, elem);
  free (page);
}
