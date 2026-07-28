#ifndef SD_H
#define SD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// SD Card — SPI driver + FatFs filesystem
// ============================================================================

// Initialize SD card on shared SPI bus
bool sd_init(void);

// Mount/unmount filesystem
bool sd_mount(void);
void sd_unmount(void);
bool sd_is_mounted(void);

// File operations
bool sd_file_exists(const char *path);
bool sd_mkdir(const char *path);

#ifdef __cplusplus
}
#endif

#endif // SD_H
