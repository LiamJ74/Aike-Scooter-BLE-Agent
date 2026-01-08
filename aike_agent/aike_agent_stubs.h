#ifndef AIKE_AGENT_STUBS_H
#define AIKE_AGENT_STUBS_H

#include <stdint.h>
#include <stdbool.h>

// Shadow Types to avoid conflict with system headers while allowing structure access
// We use "AikeGap" prefix.

typedef enum {
    AikeGapEventTypeAdvReport = 0,
    // Add other events if needed mapping
} AikeGapEventType;

typedef struct {
    uint8_t* data;
    uint8_t data_len;
    uint8_t address[6];
} AikeGapAdvReport;

typedef union {
    AikeGapAdvReport adv_report;
} AikeGapEventData;

typedef struct {
    AikeGapEventType type;
    AikeGapEventData data;
} AikeGapEvent;

typedef struct {
    uint8_t addr[6];
    uint8_t type;
} AikeBdAddr;

#ifndef BLE_SCAN_ACTIVE
#define BLE_SCAN_ACTIVE 1
#endif

#ifndef BLE_SCAN_INTERVAL_DEFAULT
#define BLE_SCAN_INTERVAL_DEFAULT 0x0060
#endif

#ifndef BLE_SCAN_WINDOW_DEFAULT
#define BLE_SCAN_WINDOW_DEFAULT 0x0030
#endif

// Custom prototypes that take void* to bypass strict type checking
// This assumes the calling convention matches (which it does for pointers and simple structs usually)
// The actual functions in the binary will take standard GapEvent types.
// We just need to trick the compiler.

void ble_gap_scan_start(uint8_t mode, uint16_t interval, uint16_t window, void* callback, void* context);
void ble_gap_scan_stop();
void ble_gap_connect(const void* peer_addr);

// GATT Client stubs
void ble_gatt_client_read_value(uint16_t conn_handle, uint16_t handle);
void ble_gatt_client_write_value(uint16_t conn_handle, uint16_t handle, const uint8_t* data, uint16_t len);

#endif
