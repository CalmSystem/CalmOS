#ifndef DISK_H_
#define DISK_H_

#include "stdint.h"
#include "stddef.h"

/** Offset from disk base address */
typedef uint32_t addr_t;

/** Polymorphic disk access */
struct disk_t {
  void (*write) (void* arg, addr_t addr, const void* src, size_t n);
  void (* read) (void* arg, void* dst, addr_t addr, size_t n);
  const void* (*view)(void* arg, addr_t addr, size_t* size);
  void* arg;
};
void disk_write(struct disk_t* d, addr_t addr, const void* src, size_t n);
void disk_read(struct disk_t* d, void* dst, addr_t addr, size_t n);
/** Reference to disk buffer is invalidated by read/write or interrupt. size is readable length is bytes */
const void* disk_view(struct disk_t* d, addr_t addr, size_t* size);

void new_mem_disk(struct disk_t*, size_t size, int erase);

#endif /*DISK_H_*/
