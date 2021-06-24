#ifndef FAT16_H_
#define FAT16_H_

#include "filesystem.h"

/** Load FAT16 fs from disk. */
void load_fat16(struct filesystem_t*);

/** Create new in memory FAT16 fs on disk */
void new_fat16(struct filesystem_t*, size_t sectorCount);

#endif /*FAT16_H_*/
