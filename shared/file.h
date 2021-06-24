#ifndef __FILE_H__
#define __FILE_H__

#include "stddef.h"
#include "stdint.h"

#define PACKED __attribute__((__packed__))

/** Time packed as FAT. Multiply seconds by 2. */
struct fat_time_t {
  uint8_t seconds : 5;
  // offset of packed bit-field ‘minutes’ has changed in GCC 4.4
  uint16_t minutes : 6;
  uint8_t hour : 5;
} PACKED;
#define FAT_YEAR_OFFSET 1980
/** Date packed as FAT */
struct fat_date_t {
  uint8_t day : 5;
  uint16_t month : 4;
  uint8_t year : 7;
} PACKED;
/** Date and time packed as FAT */
struct fat_datetime_t {
  struct fat_time_t time;
  struct fat_date_t date;
} PACKED;

/** File attributs as FAT */
enum file_attr {
  FILE_READ_ONLY = 0x01,
  FILE_HIDDEN = 0x02,
  FILE_DIRECTORY = 0x10
};

#define FILE_SHORTNAME_SIZE 31
/** File descriptor */
typedef struct FILE {
  /** Name or first part of it */
  char name[FILE_SHORTNAME_SIZE+1];
  /** Content size in bytes  */
  uint32_t size;
  /** Entry flags see file_attr */
  enum file_attr attribs;
  struct fat_datetime_t createdAt;
  struct fat_date_t accessAt;
  struct fat_datetime_t modifiedAt;
  /** Internal value */
  uint32_t parentCluster;
  /** Internal value */
  uint32_t clusterIndex;
  /** Internal value */
  uint32_t entryAddr;
} FILE;

/** Directory descriptor */
typedef struct DIR {
  /** Internal value */
  uint32_t clusterIndex;
} DIR;

#endif