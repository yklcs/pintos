#ifndef VM_VM_H
#define VM_VM_H

#include <stdbool.h>
#include "filesys/file.h"
#include "threads/interrupt.h"
#include "vm/addr.h"

void vm_init (void);
bool vm_process_init (void);
void vm_process_exit (void);

bool vm_map_file (vm_upage upage, bool writable, struct file *file, off_t ofs,
                  uint32_t read_bytes, uint32_t zero_bytes, bool mmap);
bool vm_map_zero (vm_upage upage, bool writable);
bool vm_load (vm_upage upage);
bool vm_fault (void *uaddr);

#define STACK_LIMIT (1 << 23)
bool vm_grow_stack (void *uaddr, void *esp);

#endif /* vm/vm.h */
