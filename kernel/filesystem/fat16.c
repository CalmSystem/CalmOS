#include "debug.h"
#include "stdbool.h"
#include "stdint.h"
#include "string.h"
#include "fat16.h"

/** initial FAT16 library (only root directory supported) is based on
 * https://github.com/pdoane/osdev by Patrick Doane and others (zlib License) */

#define SECTOR_SIZE 512

struct bios_block_t {
  uint8_t jump[3];
  uint8_t oem[8];
  uint16_t bytesPerSector;
  uint8_t sectorsPerCluster;
  uint16_t reservedSectorCount;
  uint8_t fatCount;
  uint16_t rootEntryCount;
  uint16_t sectorCount;
  uint8_t mediaType;
  uint16_t sectorsPerFat;
  uint16_t sectorsPerTrack;
  uint16_t headCount;
  uint32_t hiddenSectorCount;
  uint32_t largeSectorCount;

  // Extended block
  uint8_t driveNumber;
  uint8_t flags;
  uint8_t signature;
  uint32_t volumeId;
  uint8_t volumeLabel[11];
  uint8_t fileSystem[8];
} PACKED;

/** FAT16 entry. Following conventions of DOS 7.0 */
struct dir_entry_t {
  /** Short file name */
  uint8_t name[8];
  /** Short file extension */
  uint8_t ext[3];
  /** Entry flags see file_attr */
  uint8_t attribs;
  uint8_t reserved;
  uint8_t createdAtMs;
  struct fat_datetime_t createdAt;
  struct fat_date_t accessAt;
  uint16_t extendedAttribsIndex;
  struct fat_datetime_t modifiedAt;
  /** Index of first data cluster */
  uint16_t clusterIndex;
  uint32_t fileSize;
} PACKED;

/** Position of char in dir_entry_t */
const uint8_t lfn_chars[] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

enum fat_attr {
  FAT_READ_ONLY = 0x01,
  FAT_HIDDEN = 0x02,
  FAT_SYSTEM = 0x04,
  FAT_VOLUME_ID = 0x08,
  FAT_DIRECTORY = 0x10,
  FAT_ARCHIVE = 0x20,
  /** Part of long file name */
  FAT_LFN = FAT_READ_ONLY | FAT_HIDDEN | FAT_SYSTEM | FAT_VOLUME_ID
};

#define ENTRY_AVAILABLE 0x00
#define ENTRY_ERASED 0xe5

#define WITH_BPB(bpb, d) \
size_t __bpb_size; \
const struct bios_block_t *bpb = (struct bios_block_t *)disk_view(d, 0, &__bpb_size); \
assert(bpb && __bpb_size >= sizeof(struct bios_block_t));

uint32_t fat_get_total_sector_count(struct disk_t *d) {
  WITH_BPB(bpb, d);
  if (bpb->sectorCount) {
    return bpb->sectorCount;
  } else {
    return bpb->largeSectorCount;
  }
}
uint32_t fat_get_meta_sector_count(struct disk_t *d) {
  WITH_BPB(bpb, d);
  return bpb->reservedSectorCount + bpb->fatCount * bpb->sectorsPerFat +
         (bpb->rootEntryCount * sizeof(struct dir_entry_t)) / bpb->bytesPerSector;
}
uint32_t fat_get_cluster_count(struct disk_t* d) {
  uint32_t totalSectorCount = fat_get_total_sector_count(d);
  uint32_t metaSectorCount = fat_get_meta_sector_count(d);
  uint32_t dataSectorCount = totalSectorCount - metaSectorCount;

  WITH_BPB(bpb, d);
  return dataSectorCount / bpb->sectorsPerCluster;
}

addr_t fat_get_root_table_addr(struct disk_t *d) {
  WITH_BPB(bpb, d);
  return (bpb->reservedSectorCount + 0 * bpb->sectorsPerFat) *
                    bpb->bytesPerSector;
}

addr_t fat_get_cluster_addr(struct disk_t *d, uint32_t clusterIndex) {
  WITH_BPB(bpb, d);
  uint32_t fatOffset =
      (bpb->reservedSectorCount + bpb->fatCount * bpb->sectorsPerFat) *
      bpb->bytesPerSector;
  if (clusterIndex < 2) return fatOffset;
  return fatOffset + bpb->rootEntryCount * sizeof(struct dir_entry_t) +
         (clusterIndex - 2) * (bpb->sectorsPerCluster * bpb->bytesPerSector);
}

uint16_t fat_get_cluster_value(struct disk_t *d, uint32_t clusterIndex) {
  uint32_t fatOffset = fat_get_root_table_addr(d);
  assert(clusterIndex < fat_get_cluster_count(d));
  uint16_t value;
  disk_read(d, &value, fatOffset + clusterIndex * sizeof(value), sizeof(value));
  return value;
}
void fat_set_cluster_value(struct disk_t *d, uint32_t clusterIndex, uint16_t value) {
  uint32_t fatOffset = fat_get_root_table_addr(d);
  assert(clusterIndex < fat_get_cluster_count(d));
  disk_write(d, fatOffset + clusterIndex * sizeof(value), &value, sizeof(value));
}

uint16_t fat_find_free_cluster(struct disk_t *d) {
  uint32_t clusterCount = fat_get_cluster_count(d);
  uint32_t fatOffset = fat_get_root_table_addr(d);

  for (uint32_t clusterIndex = 2; clusterIndex < clusterCount; ++clusterIndex) {
    uint16_t data;
    disk_read(d, &data, fatOffset + clusterIndex * sizeof(data), sizeof(data));
    if (data == 0) return clusterIndex;
  }

  return 0;
}

void fat_update_cluster(struct disk_t *d, uint32_t clusterIndex, uint16_t value) {
  WITH_BPB(bpb, d);
  uint8_t fatCount = bpb->fatCount;
  for (uint32_t fatIndex = 0; fatIndex < fatCount; fatIndex++) {
    fat_set_cluster_value(d, clusterIndex, value);
  }
}

bool fat_format(struct disk_t *d, struct bios_block_t *bpb) {
  uint8_t *boot_sector = (uint8_t*)bpb;
  // Validate signature
  if (boot_sector[0x1fe] != 0x55 || boot_sector[0x1ff] != 0xaa) {
    return false;
  }

  // Copy to sector 0
  disk_write(d, 0, boot_sector, bpb->bytesPerSector);

  // Initialize clusters
  uint32_t clusterCount = fat_get_cluster_count(d);
  assert(clusterCount >= 2);

  fat_update_cluster(d, 0, 0xff00 | bpb->mediaType);  // media type
  fat_update_cluster(d, 1, 0xffff);  // end of chain cluster marker

  for (uint32_t clusterIndex = 2; clusterIndex < clusterCount; ++clusterIndex) {
    fat_update_cluster(d, clusterIndex, 0x0000);
  }

  return true;
}

addr_t fat_find_entry(struct disk_t *d, uint32_t clusterIndex, bool free, int32_t skip, int32_t* index) {
  WITH_BPB(bpb, d);
  uint16_t rootEntryCount = bpb->rootEntryCount;

  uint32_t start = fat_get_cluster_addr(d, clusterIndex);

  for (int32_t offset = skip; offset < rootEntryCount; offset++) {
    struct dir_entry_t *entry = (struct dir_entry_t *)disk_view( d,
        start + offset * sizeof(struct dir_entry_t) +
            offsetof(struct dir_entry_t, name), NULL);
    if (free ^ (entry->name[0] != ENTRY_AVAILABLE && entry->name[0] != ENTRY_ERASED)) {
      if (index) *index = offset;
      return start + offset * sizeof(struct dir_entry_t);
    }
  }

  return 0;
}
addr_t fat_find_free_entry(struct disk_t *d, uint32_t clusterIndex) {
  return fat_find_entry(d, clusterIndex, true, 0, NULL);
}

static int toupper(int c) {
  if (c > 96 && c < 123) {
    return c - 32;
  }
  return c;
}
static void set_padded_string(uint8_t *dst, uint32_t dstLen, const char *src, uint32_t srcLen) {
  if (src) {
    if (srcLen > dstLen) {
      memcpy(dst, src, dstLen);
    } else {
      memcpy(dst, src, srcLen);
      memset(dst + srcLen, ' ', dstLen - srcLen);
    }

    for (uint32_t i = 0; i < dstLen; i++) {
      dst[i] = toupper(dst[i]);
    }
  } else {
    memset(dst, ' ', dstLen);
  }
}

void fat_split_path(uint8_t dstName[8], uint8_t dstExt[3], const char *path) {
  const char *name = strrchr(path, '/');
  if (name) {
    name = name + 1;
  } else {
    name = path;
  }

  uint32_t nameLen = strlen(name);

  char *ext = 0;
  uint32_t extLen = 0;
  char *p = strchr(name, '.');
  if (p) {
    nameLen = p - name;
    ext = p + 1;
    extLen = strlen(ext);
  }

  set_padded_string(dstName, 8, name, nameLen);
  set_padded_string(dstExt, 3, ext, extLen);
}

void fat_update_dir_entry(struct disk_t* d, addr_t addr, uint16_t clusterIndex, const uint8_t name[8], const uint8_t ext[3], uint32_t fileSize) {
  struct dir_entry_t *entry = (struct dir_entry_t *)disk_view(d, addr, NULL);
  entry->clusterIndex = clusterIndex;
  memcpy(entry->name, name, sizeof((struct dir_entry_t){0}).name);
  memcpy(entry->ext, ext, sizeof((struct dir_entry_t){0}).ext);
  entry->fileSize = fileSize;
}
void fat_remove_dir_entry(struct disk_t* d, addr_t addr) {
  const char available = ENTRY_AVAILABLE;
  disk_write(d, addr + offsetof(struct dir_entry_t, name), &available, sizeof(available));
}

void fat_remove_data(struct disk_t *d, uint32_t fatIndex, uint32_t clusterIndex) {
  assert(clusterIndex != 0);

  uint16_t endOfChainValue = fat_get_cluster_value(d, 1);
  while (clusterIndex != endOfChainValue) {
    uint16_t nextClusterIndex = fat_get_cluster_value(d, clusterIndex);
    fat_update_cluster(d, clusterIndex, fatIndex);
    clusterIndex = nextClusterIndex;
  }
}
uint16_t fat_add_data(struct disk_t *d, uint32_t fatIndex, const void *data, uint32_t len) {
  WITH_BPB(bpb, d);
  uint32_t bytesPerCluster = bpb->sectorsPerCluster * bpb->bytesPerSector;

  // Skip empty files
  if (len == 0) return 0;

  uint16_t endOfChainValue = fat_get_cluster_value(d, 1);

  uint16_t prevClusterIndex = 0;
  uint16_t rootClusterIndex = 0;

  // Copy data one cluster at a time.
  const uint8_t *p = (const uint8_t *)data;
  const uint8_t *end = p + len;
  while (p < end) {
    // Find a free cluster
    uint16_t clusterIndex = fat_find_free_cluster(d);
    if (clusterIndex == 0) {
      // Ran out of disk space, free allocated clusters
      if (rootClusterIndex != 0) {
        fat_remove_data(d, fatIndex, rootClusterIndex);
      }

      return 0;
    }

    // Determine amount of data to copy
    uint32_t count = end - p;
    if (count > bytesPerCluster) count = bytesPerCluster;

    // Transfer bytes into image at cluster location
    uint32_t offset = fat_get_cluster_addr(d, clusterIndex);
    disk_write(d, offset, p, count);
    p += count;

    // Update FAT clusters
    fat_update_cluster(d, clusterIndex, endOfChainValue);
    if (prevClusterIndex) {
      fat_update_cluster(d, prevClusterIndex, clusterIndex);
    } else {
      rootClusterIndex = clusterIndex;
    }

    prevClusterIndex = clusterIndex;
  }

  return rootClusterIndex;
}
int fat_read_data(struct disk_t *d, uint32_t clusterIndex, void* data, uint32_t len, size_t skip) {
  assert(clusterIndex != 0);
  WITH_BPB(bpb, d);
  uint32_t bytesPerCluster = bpb->sectorsPerCluster * bpb->bytesPerSector;

  // Skip empty files
  if (len == 0) return 0;

  uint16_t endOfChainValue = fat_get_cluster_value(d, 1);
  uint32_t read_size = 0;
  uint8_t *p = (uint8_t *)data;

  // Copy data one cluster at a time.
  while (clusterIndex != endOfChainValue && len > (uint32_t)(p-(uint8_t*)data)) {
    if (read_size + bytesPerCluster > skip) {
      // Determine amount of data to copy
      uint32_t start = read_size > skip ? 0 : skip - read_size;
      uint32_t count = len - (p-(uint8_t*)data);
      if (count > bytesPerCluster) count = bytesPerCluster;

      // Transfer bytes into image at cluster location
      uint32_t offset = fat_get_cluster_addr(d, clusterIndex);
      disk_read(d, p, offset+start, count);
      p += count;
    }
    read_size += bytesPerCluster;
    clusterIndex = fat_get_cluster_value(d, clusterIndex);
  }
  return p - (uint8_t *)data;
}
int fat_write_data(struct disk_t *d, uint32_t clusterIndex, const void* data, uint32_t len, size_t skip) {
  assert(clusterIndex != 0);
  WITH_BPB(bpb, d);
  uint32_t bytesPerCluster = bpb->sectorsPerCluster * bpb->bytesPerSector;

  // Skip empty files
  if (len == 0) return 0;

  uint16_t endOfChainValue = fat_get_cluster_value(d, 1);
  uint32_t read_size = 0;
  uint8_t *p = (uint8_t *)data;

  // Copy data one cluster at a time.
  while (clusterIndex != endOfChainValue && len > (uint32_t)(p-(uint8_t*)data)) {
    if (read_size + bytesPerCluster > skip) {
      // Determine amount of data to copy
      uint32_t start = read_size > skip ? 0 : skip - read_size;
      uint32_t count = len - (p-(uint8_t*)data);
      if (count > bytesPerCluster) count = bytesPerCluster;

      // Transfer bytes into image at cluster location
      uint32_t offset = fat_get_cluster_addr(d, clusterIndex);
      disk_write(d, offset+start, p, count);
      p += count;
    }
    read_size += bytesPerCluster;
    clusterIndex = fat_get_cluster_value(d, clusterIndex);
  }
  return p - (uint8_t *)data;
}

addr_t fat_add_file(struct disk_t* d, uint32_t clusterIndex, uint32_t fatIndex, const char *path, const void *data, uint32_t size) {
  // Find Directory Entry
  addr_t entry = fat_find_free_entry(d, clusterIndex);
  if (!entry) return 0;

  // Add File
  uint16_t rootClusterIndex = fat_add_data(d, fatIndex, data, size);
  if (!rootClusterIndex) return 0;

  // Update Directory Entry
  uint8_t name[8];
  uint8_t ext[3];
  fat_split_path(name, ext, path);

  fat_update_dir_entry(d, entry, rootClusterIndex, name, ext, size);
  return entry;
}
void fat_remove_file(struct disk_t *d, addr_t addr) {
  uint16_t clusterIndex;
  disk_read(d, &clusterIndex, addr + offsetof(struct dir_entry_t, clusterIndex), sizeof(clusterIndex));
  fat_remove_data(d, 0, clusterIndex);
  fat_remove_dir_entry(d, addr);
}

DIR fat_fs_root(struct filesystem_t* self) {
  (void)self;
  DIR dir = {.clusterIndex = 0};
  return dir;
}
void fat_fs_file_name(struct filesystem_t *self, const FILE *f, char *name, size_t len) {
  if (!(f && name)) return;

  size_t cur = 0;
  const struct dir_entry_t *entry = disk_view(self->disk, f->entryAddr, NULL);

  size_t name_len = (sizeof((struct dir_entry_t){0}).name);
  while (name_len && entry->name[name_len-1] == ' ') { name_len--; }

  if (name_len > len) name_len = len;
  memcpy(name, entry->name, name_len);
  cur += name_len;

  size_t ext_len = (sizeof((struct dir_entry_t){0}).ext);
  while (ext_len && entry->ext[ext_len-1] == ' ') { ext_len--; }
  if (ext_len && len > name_len) {
    name[name_len] = '.';
    cur += 1;
    size_t ext_start = name_len + 1;
    if (ext_start + ext_len > len) ext_len = len - ext_start;
    memcpy(&name[ext_start], entry->ext, ext_len);
    cur += ext_len;
  }
  name[cur] = '\0';

  // Long file name
  cur = 0;
  addr_t lfnAddr = f->entryAddr;
  while (cur < len) {
    lfnAddr -= sizeof(struct dir_entry_t);
    entry = disk_view(self->disk, lfnAddr, NULL);

    if (entry->attribs != FAT_LFN) break;
    for (uint8_t i = 0; i < sizeof(lfn_chars)/sizeof(lfn_chars[0]) && cur < len; i++) {
      name[cur] = *((char *)entry + lfn_chars[i]);
      cur++;
    }
  }
  if (cur) name[cur] = '\0';

  return;
}
int fat_fs_list(struct filesystem_t *self, const DIR dir, FILE *files, size_t nfiles, size_t offset) {
  int32_t current = offset;
  for (size_t i = 0; i < nfiles; i++) {
    bool visible = false;
    do {
      addr_t addr = fat_find_entry(self->disk, dir.clusterIndex, false, current, &current);
      if (!addr) return i;

      current++;
      const struct dir_entry_t* entry = disk_view(self->disk, addr, NULL);
      if (entry->attribs & (FAT_SYSTEM | FAT_VOLUME_ID)) continue;
      visible = true;

      files[i].size = entry->fileSize;
      files[i].attribs = 0;
      if (entry->attribs & FAT_READ_ONLY) files[i].attribs |= FILE_READ_ONLY;
      if (entry->attribs & FAT_HIDDEN) files[i].attribs |= FILE_HIDDEN;
      if (entry->attribs & FAT_DIRECTORY) files[i].attribs |= FILE_DIRECTORY;

      files[i].createdAt = entry->createdAt;
      files[i].accessAt = entry->accessAt;
      files[i].modifiedAt = entry->modifiedAt;
      files[i].parentCluster = dir.clusterIndex;
      files[i].clusterIndex = entry->clusterIndex;
      files[i].entryAddr = addr;

      fat_fs_file_name(self, &files[i], &files[i].name[0], FILE_SHORTNAME_SIZE);
    } while (!visible);
  }
  return nfiles;
}
int fat_fs_read(struct filesystem_t *self, void *dst, const FILE *f, size_t offset, size_t len) {
  if (len > f->size - offset) len = f->size - offset;
  return fat_read_data(self->disk, f->clusterIndex, dst, len, offset);
}
int fat_fs_write(struct filesystem_t *self, const FILE *f, size_t offset, const void *src, size_t len) {
  if (len > f->size - offset) len = f->size - offset;
  return fat_write_data(self->disk, f->clusterIndex, src, len, offset);
}

void load_fat16(struct filesystem_t *fs) {
  assert(sizeof(struct bios_block_t) == 62);
  assert(sizeof(struct dir_entry_t) == 32);
  assert(sizeof(struct fat_date_t) == 2);
  assert(sizeof(struct fat_datetime_t) == 4);

  fs->root = fat_fs_root;
  fs->list = fat_fs_list;
  fs->file_name = fat_fs_file_name;
  fs->read = fat_fs_read;
  fs->write = fat_fs_write;
}

void new_fat16(struct filesystem_t *fs, size_t sectorCount) {
  struct bios_block_t bpb = {
    .jump = {0xeb, 0x3c, 0x90},  // short jmp followed by nop
    .oem = {'F', 'A', 'T', ' ', 'T', 'E', 'S', 'T'},
    .bytesPerSector = 512,
    .sectorsPerCluster = 1,
    .reservedSectorCount = 4,
    .fatCount = 2,
    .rootEntryCount = 512,
    .sectorCount = sectorCount,
    .mediaType = 0xf8,
    .sectorsPerFat = 20,
    .sectorsPerTrack = 63,
    .headCount = 255,
    .hiddenSectorCount = 0,
    .largeSectorCount = 0,
    .driveNumber = 0x80,
    .flags = 0x00,
    .signature = 0x29,
    .volumeId = 0xa0a1a2a3,
    .volumeLabel = {'V', 'o', 'l', 'u', 'm', 'e', ' ', 'N', 'a', 'm', 'e'},
    .fileSystem = {'F', 'A', 'T', '1', '6', ' ', ' ', ' '},
  };

  // Allocate device
  const unsigned long imageSize = bpb.sectorCount * bpb.bytesPerSector;
  new_mem_disk(fs->disk, imageSize, ENTRY_ERASED);

  // Create dummy boot sector
  uint8_t bootSector[0x200];
  memset(bootSector, 0, sizeof(bootSector));
  memcpy(bootSector, &bpb, sizeof(bpb));
  bootSector[0x1fe] = 0x55;
  bootSector[0x1ff] = 0xaa;

  // Initialize image
  load_fat16(fs);
  fat_format(fs->disk, (struct bios_block_t *)&bootSector);
}
