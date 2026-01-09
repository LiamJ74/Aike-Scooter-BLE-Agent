#ifndef AIKE_AGENT_STUBS_H
#define AIKE_AGENT_STUBS_H

#include <stdint.h>
#include <stdbool.h>

// ==========================================
// HCI / ACI Definitions for Flipper Zero (STM32WB)
// ==========================================

// Basic Types
typedef uint8_t tBleStatus;
typedef void GapSvcEventHandler;

// HCI Packet Types
#define HCI_EVENT_PKT_TYPE 0x04

// HCI Event Codes
#define HCI_LE_META_EVENT 0x3E

// LE Subevents
#define HCI_LE_CONNECTION_COMPLETE_EVENT 0x01
#define HCI_LE_ADVERTISING_REPORT_EVENT 0x02

// ACI/GAP Constants
#define OWN_ADDRESS_PUBLIC 0x00
#define DUPLICATE_FILTER_ENABLED 0x01
#define GAP_GENERAL_DISCOVERY_PROC 0x02

// Structs (Packed to match wire format)

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t data[];
} hci_uart_pckt;

typedef struct __attribute__((packed)) {
    uint8_t evt;
    uint8_t plen;
    uint8_t data[];
} hci_event_pckt;

typedef struct __attribute__((packed)) {
    uint8_t subevent;
    uint8_t data[];
} evt_le_meta_event;

typedef struct __attribute__((packed)) {
    uint8_t num_reports;
    uint8_t event_type;   // 1 byte (Offset 0)
    uint8_t address_type; // 1 byte (Offset 1)
    uint8_t address[6];   // 6 bytes (Offset 2)
    uint8_t data_length;  // 1 byte (Offset 8)
    uint8_t data[];       // Variable length
} hci_le_advertising_report_event_rp0;

typedef struct __attribute__((packed)) {
    uint8_t status;
    uint16_t handle;
    uint8_t role;
    uint8_t peer_addr_type;
    uint8_t peer_addr[6];
    uint16_t interval;
    uint16_t latency;
    uint16_t supervision_timeout;
    uint8_t master_clock_accuracy;
} evt_le_connection_complete;

// Ble Event Ack Status
typedef enum {
    BleEventNotAck,
    BleEventAck,
    BleEventAckFlowEnable,
} BleEventAckStatus;

// Function Prototypes for Hooks and ACI

// Registers a low-level HCI event handler
void* ble_event_dispatcher_register_svc_handler(BleEventAckStatus (*handler)(void* event, void* context), void* context);

// Unregisters the handler
void ble_event_dispatcher_unregister_svc_handler(void* handler);

// ACI GAP Functions
tBleStatus aci_gap_start_general_discovery_proc(uint16_t scan_interval, uint16_t scan_window, uint8_t own_address_type, uint8_t filter_duplicates);

tBleStatus aci_gap_terminate_gap_proc(uint8_t procedure_code);

tBleStatus aci_gap_create_connection(uint16_t scan_interval, uint16_t scan_window, uint8_t peer_address_type, uint8_t* peer_address, uint8_t own_address_type, uint16_t conn_interval_min, uint16_t conn_interval_max, uint16_t conn_latency, uint16_t supervision_timeout, uint16_t min_ce_length, uint16_t max_ce_length);

tBleStatus hci_disconnect(uint16_t connection_handle, uint8_t reason);

// ==========================================
// Legacy Stubs (kept for compatibility if needed)
// ==========================================

typedef enum {
    AikeGapEventTypeAdvReport = 0,
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

#endif
