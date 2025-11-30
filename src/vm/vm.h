#ifndef VM_VM_H
#define VM_VM_H

#include "threads/thread.h"
#include "vm/addr.h"

void vm_init (void);
bool vm_process_init (void);
void vm_thread_destroy (void);

bool vm_map_file (vm_upage upage, bool writable, struct file *file, off_t ofs,
                  uint32_t read_bytes, uint32_t zero_bytes, bool mmap);
bool vm_map_zero (vm_upage upage, bool writable);
bool vm_load (vm_upage upage);

#endif /* vm/vm.h */
