#ifndef AIKE_AGENT_STUBS_H
#define AIKE_AGENT_STUBS_H

#include <stdint.h>
#include <stdbool.h>

// This file provides stubs for standard Momentum/Unleashed BLE definitions
// It is guarded to avoid conflicts if the environment already provides them.

#ifndef BLE_SCAN_ACTIVE
    // Enum and Struct definitions
    typedef enum {
        GapEventAdvReport = 0,
        GapEventConnected,
        GapEventDisconnected,
        // Add other events as needed
    } GapEvent;

    typedef struct {
        uint8_t* data;
        uint8_t data_len;
        uint8_t address[6];
    } GapAdvReport;

    typedef struct {
        GapAdvReport adv_report;
    } GapEventData;

    typedef struct {
        uint8_t addr[6];
        uint8_t type;
    } bd_addr_t;

    #define BLE_SCAN_ACTIVE 1
    #define BLE_SCAN_INTERVAL_DEFAULT 0x0060
    #define BLE_SCAN_WINDOW_DEFAULT 0x0030

    // Stub function prototypes to satisfy linker/compiler in non-custom environments
    // We make them weak or just prototypes. Since we are not linking, prototypes are enough for compilation.

    void ble_gap_scan_start(uint8_t mode, uint16_t interval, uint16_t window, void* callback, void* context);
    void ble_gap_scan_stop();
    void ble_gap_connect(const bd_addr_t* peer_addr);

    // GATT Client stubs
    void ble_gatt_client_read_value(uint16_t conn_handle, uint16_t handle);
    void ble_gatt_client_write_value(uint16_t conn_handle, uint16_t handle, const uint8_t* data, uint16_t len);

#endif

#endif
