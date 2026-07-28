#ifndef MSC_H
#define MSC_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// USB Mass Storage — TinyUSB MSC device
// ============================================================================

// Initialize TinyUSB for mass storage class
void msc_init(void);

// Check if USB cable is connected (VBUS sense)
bool msc_is_connected(void);

// Enter MSC mode (blocks until USB is disconnected)
void msc_run(void);

#ifdef __cplusplus
}
#endif

#endif // MSC_H
