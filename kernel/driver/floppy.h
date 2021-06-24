#ifndef FLOPPY_H_
#define FLOPPY_H_
#include "stdbool.h"
#include "disk.h"

/** React to floppy disk controller interrupt */
void floppy_IT();

/** Check if some floppy disk is inserted */
bool has_floppy();

/** Create polymorphic disk for floppy0 if exists */
bool load_floppy(struct disk_t *);

#endif /*FLOPPY_H_*/
