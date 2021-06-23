#ifndef FLOPPY_H_
#define FLOPPY_H_
#include "stdbool.h"
#include "stdint.h"

/** React to floppy disk controller interrupt */
void floppy_IT();

/** Check if some floppy disk is inserted */
bool has_floppy();

/** Reference to floppy_dmabuf or NULL. size is readable length is bytes */
const char* floppy_read_view(uint32_t addr, uint32_t* size);
/** Read to dst if result is positive */
int floppy_read(char* dst, uint32_t addr, uint32_t size);

/** Writes from src if result is positive */
int floppy_write(uint32_t addr, const char* src, uint32_t size);

#endif /*FLOPPY_H_*/
