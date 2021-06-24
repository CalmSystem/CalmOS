#include "mem.h"
#include "string.h"
#include "disk.h"

void disk_write(struct disk_t *d, uint32_t addr, const void *src, size_t n) {
  d->write(d->arg, addr, src, n);
}
void disk_read(struct disk_t *d, void *dst, uint32_t addr, size_t n) {
  d->read(d->arg, dst, addr, n);
}
const void *disk_view(struct disk_t *d, addr_t addr, size_t *size) {
  return d->view(d->arg, addr, size);
}

void mem_disk_write(void *arg, uint32_t addr, const void *src, size_t n) {
  memcpy(arg + addr, src, n);
}
void mem_disk_read(void *arg, void *dst, uint32_t addr, size_t n) {
  memcpy(dst, arg + addr, n);
}
const void *mem_disk_view(void *arg, uint32_t addr, uint32_t *size) {
  if (size) *size = UINT32_MAX;
  return arg + addr;
}
void new_mem_disk(struct disk_t *d, size_t size, int erase) {
  d->arg = mem_alloc(size);
  d->read = mem_disk_read;
  d->write = mem_disk_write;
  d->view = mem_disk_view;
  memset(d->arg, erase, size);
}
