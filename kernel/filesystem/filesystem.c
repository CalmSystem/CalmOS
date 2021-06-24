#include "floppy.h"
#include "fat16.h"
#include "stdio.h"

struct disk_t root_disk = {0};
struct filesystem_t root_fs = {0};
void setup_filesystem() {
  // NOTE: Assume filesystem is FAT16

  // Load floppy or use in memory disk
  root_fs.disk = &root_disk;
  if (load_floppy(&root_disk)) {
    load_fat16(&root_fs);
  } else {
    printf("Using in memory disk !\n");
    new_fat16(&root_fs, 128);
  }
}

DIR fs_root() { return root_fs.root(&root_fs); }
int fs_list(const DIR dir, FILE *files, size_t nfiles, size_t offset) {
  return root_fs.list(&root_fs, dir, files, nfiles, offset);
}
void fs_file_name(const FILE *f, char *name, size_t len) {
  return root_fs.file_name(&root_fs, f, name, len);
}
int fs_read(void *dst, const FILE *f, size_t offset, size_t len) {
  return root_fs.read(&root_fs, dst, f, offset, len);
}
int fs_write(const FILE *f, size_t offset, const void *src, size_t len) {
  return root_fs.write(&root_fs, f, offset, src, len);
}
