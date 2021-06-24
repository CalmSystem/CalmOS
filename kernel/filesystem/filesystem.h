#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "disk.h"
#include "file.h"

/** Load floppy or init in memory */
void setup_filesystem();

/** Polymorphic filesystem. For function details see corresponding fs_ method */
struct filesystem_t {
  struct disk_t* disk;
  DIR (*root)(struct filesystem_t* self);
  int (*list)(struct filesystem_t *self, const DIR dir, FILE *files, size_t nfiles, size_t offset);
  void (*file_name)(struct filesystem_t *self, const FILE *f, char *name, size_t len);
  int (*read)(struct filesystem_t *self, void *dst, const FILE *f, size_t offset, size_t len);
  int (*write)(struct filesystem_t *self, const FILE *f, size_t offset, const void *src, size_t len);
};

/** Get top level folder */
DIR fs_root();
/** Get file list in directory.
 * Return error or number of files.
 * Takes an array of file_t */
int fs_list(const DIR dir, FILE *files, size_t nfiles, size_t offset);
/** Get full filename. len does not include \0 */
void fs_file_name(const FILE *f, char *name, size_t len);
/** Read file part. Return error or readded size */
int fs_read(void *dst, const FILE *f, size_t offset, size_t len);
/** Write file part. Return error or written size */
int fs_write(const FILE *f, size_t offset, const void *src, size_t len);

#endif /*FILESYSTEM_H_*/
