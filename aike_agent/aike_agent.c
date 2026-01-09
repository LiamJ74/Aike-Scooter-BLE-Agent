#include "aike_agent.h"
#include "aike_agent_stubs.h" // Includes ACI/HCI definitions
#include "sha1.h"
#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <furi_hal.h>

// If furi_hal_bt.h is not found locally, we assume it's available in the build environment
// or we use forward declarations if needed.
#include <furi_hal_bt.h>

// =============================================================================
// ACI Implementation (Missing Symbols Fix)
// =============================================================================
// These functions are not exported by the firmware (or not linked), so we
// implement them using the exported hci_send_req function.

struct hci_request {
    uint16_t ogf;
    uint16_t ocf;
    int event;
    void* cparam;
    int clen;
    void* rparam;
    int rlen;
};

extern int hci_send_req(struct hci_request* req, uint8_t async);

tBleStatus aci_gap_start_general_discovery_proc(uint16_t scan_interval, uint16_t scan_window, uint8_t own_address_type, uint8_t filter_duplicates) {
    struct __attribute__((packed)) {
        uint16_t scan_interval;
        uint16_t scan_window;
        uint8_t own_addr_type;
        uint8_t filter_duplicates;
    } params = {
        .scan_interval = scan_interval,
        .scan_window = scan_window,
        .own_addr_type = own_address_type,
        .filter_duplicates = filter_duplicates
    };

    uint8_t status = 0;
    struct hci_request req = {
        .ogf = 0x3F,
        .ocf = 0x82,
        .event = 0x0E, // Command Complete
        .cparam = &params,
        .clen = sizeof(params),
        .rparam = &status,
        .rlen = 1
    };

    if(hci_send_req(&req, 0) < 0) return 0xFF;
    return status;
}

tBleStatus aci_gap_create_connection(uint16_t scan_interval, uint16_t scan_window, uint8_t peer_address_type, uint8_t* peer_address, uint8_t own_address_type, uint16_t conn_interval_min, uint16_t conn_interval_max, uint16_t conn_latency, uint16_t supervision_timeout, uint16_t min_ce_length, uint16_t max_ce_length) {
    struct __attribute__((packed)) {
        uint16_t scan_interval;
        uint16_t scan_window;
        uint8_t peer_address_type;
        uint8_t peer_address[6];
        uint8_t own_address_type;
        uint16_t conn_interval_min;
        uint16_t conn_interval_max;
        uint16_t conn_latency;
        uint16_t supervision_timeout;
        uint16_t min_ce_length;
        uint16_t max_ce_length;
    } params;

    params.scan_interval = scan_interval;
    params.scan_window = scan_window;
    params.peer_address_type = peer_address_type;
    memcpy(params.peer_address, peer_address, 6);
    params.own_address_type = own_address_type;
    params.conn_interval_min = conn_interval_min;
    params.conn_interval_max = conn_interval_max;
    params.conn_latency = conn_latency;
    params.supervision_timeout = supervision_timeout;
    params.min_ce_length = min_ce_length;
    params.max_ce_length = max_ce_length;

    uint8_t status = 0;
    struct hci_request req = {
        .ogf = 0x3F,
        .ocf = 0x43, // ACI_GAP_CREATE_CONNECTION
        .event = 0x0F, // Command Status (Usually 0x0F for async commands like Create Connection)
        .cparam = &params,
        .clen = sizeof(params),
        .rparam = &status,
        .rlen = 1
    };

    if(hci_send_req(&req, 0) < 0) return 0xFF;
    return status;
}

tBleStatus hci_disconnect(uint16_t connection_handle, uint8_t reason) {
    struct __attribute__((packed)) {
        uint16_t connection_handle;
        uint8_t reason;
    } params = {
        .connection_handle = connection_handle,
        .reason = reason
    };

    uint8_t status = 0;
    struct hci_request req = {
        .ogf = 0x01,
        .ocf = 0x06, // HCI_DISCONNECT
        .event = 0x0F, // Command Status
        .cparam = &params,
        .clen = sizeof(params),
        .rparam = &status,
        .rlen = 1
    };

    if(hci_send_req(&req, 0) < 0) return 0xFF;
    return status;
}

tBleStatus aci_gap_terminate_gap_proc(uint8_t procedure_code) {
    uint8_t status = 0;
    struct hci_request req = {
        .ogf = 0x3F,
        .ocf = 0x83,
        .event = 0x0E,
        .cparam = &procedure_code,
        .clen = 1,
        .rparam = &status,
        .rlen = 1
    };

    if(hci_send_req(&req, 0) < 0) return 0xFF;
    return status;
}

#define TAG "AikeAgent"

// Master Key: 20x 0xFF
static const uint8_t MASTER_KEY[20] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const char* aike_name_prefix = "AIKE";

// Function prototypes
static void aike_agent_start_scan(AikeAgentApp* app);
static void aike_agent_stop_scan(AikeAgentApp* app);
static void aike_agent_connect(AikeAgentApp* app, const char* mac_address);
static void aike_agent_authenticate(AikeAgentApp* app);
static void aike_agent_send_command(AikeAgentApp* app, const uint8_t* command, uint8_t len);

static void aike_agent_scan_callback(void* context, uint32_t index);
static void aike_agent_control_callback(void* context, AikeCommand command);
static bool aike_agent_view_dispatcher_custom_event_callback(void* context, uint32_t event);
static bool aike_agent_view_dispatcher_navigation_event_callback(void* context);

// HCI Event Handler
static BleEventAckStatus aike_hci_event_handler(void* event, void* context);

AikeAgentApp* aike_agent_app_alloc() {
    AikeAgentApp* app = malloc(sizeof(AikeAgentApp));

    // GUI
    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, aike_agent_view_dispatcher_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, aike_agent_view_dispatcher_navigation_event_callback);

    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->device_count = 0;
    app->ble_event_handler = NULL;

    // Views
    app->scan_view = aike_scan_view_alloc();
    aike_scan_view_set_callback(app->scan_view, aike_agent_scan_callback, app);
    view_dispatcher_add_view(app->view_dispatcher, AikeAgentViewScan, aike_scan_view_get_view(app->scan_view));

    app->control_view = aike_control_view_alloc();
    aike_control_view_set_callback(app->control_view, aike_agent_control_callback, app);
    view_dispatcher_add_view(app->view_dispatcher, AikeAgentViewControl, aike_control_view_get_view(app->control_view));

    // Initial view
    view_dispatcher_switch_to_view(app->view_dispatcher, AikeAgentViewScan);
    app->current_view = AikeAgentViewScan;
    aike_agent_start_scan(app);

    return app;
}

void aike_agent_app_free(AikeAgentApp* app) {
    aike_agent_stop_scan(app);

    view_dispatcher_remove_view(app->view_dispatcher, AikeAgentViewControl);
    view_dispatcher_remove_view(app->view_dispatcher, AikeAgentViewScan);

    aike_control_view_free(app->control_view);
    aike_scan_view_free(app->scan_view);

    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);

    furi_mutex_free(app->mutex);
    free(app);
}

// HCI Event Handler to capture Adv Reports and Connection Events
static BleEventAckStatus aike_hci_event_handler(void* event, void* context) {
    AikeAgentApp* app = (AikeAgentApp*)context;
    hci_uart_pckt* pckt = (hci_uart_pckt*)event;

    if(pckt->type != HCI_EVENT_PKT_TYPE) return BleEventNotAck;

    hci_event_pckt* evt_pckt = (hci_event_pckt*)pckt->data;
    
    if(evt_pckt->evt == HCI_LE_META_EVENT) {
        evt_le_meta_event* meta_evt = (evt_le_meta_event*)evt_pckt->data;

        if(meta_evt->subevent == HCI_LE_CONNECTION_COMPLETE_EVENT) {
            evt_le_connection_complete* cc = (evt_le_connection_complete*)meta_evt->data;
            if (cc->status == 0) {
                // Connected successfully
                FURI_LOG_I(TAG, "Connected! Handle: 0x%04X", cc->handle);
                app->connection_handle = cc->handle; // Need to add this to struct
                aike_control_view_set_status(app->control_view, "Connected");

                // Transition to Auth (Write Challenge)
                // TODO: Need GATT Handle. For now, we update status.
                aike_agent_authenticate(app);
            } else {
                 FURI_LOG_E(TAG, "Connection Failed: 0x%02X", cc->status);
                 aike_control_view_set_status(app->control_view, "Connection Failed");
            }
            return BleEventNotAck;
        } else if(meta_evt->subevent == HCI_LE_ADVERTISING_REPORT_EVENT) {
            hci_le_advertising_report_event_rp0* adv_report = (hci_le_advertising_report_event_rp0*)meta_evt->data;
            
            // We only handle one report per event for simplicity, though num_reports can be > 1
            if (adv_report->num_reports > 0) {
                uint8_t* adv_data = adv_report->data;
                uint8_t adv_data_len = adv_report->data_length;
                
                uint8_t i = 0;
                char name[32] = {0};
                bool found_aike = false;

                while(i < adv_data_len && i < 31) { // Safety check
                    uint8_t len = adv_data[i];
                    if(len == 0) break;
                    if (i + 1 >= adv_data_len) break; // Overflow protection
                    
                    uint8_t type = adv_data[i + 1];

                    if(type == 0x09 || type == 0x08) { // Complete or Short Local Name
                        uint8_t name_len = len - 1;
                        if(name_len > 31) name_len = 31;
                        if (i + 2 + name_len > adv_data_len) break; // Overflow protection

                        memcpy(name, &adv_data[i + 2], name_len);
                        name[name_len] = '\0';

                        if(strncmp(name, aike_name_prefix, strlen(aike_name_prefix)) == 0) {
                            found_aike = true;
                        }
                        break;
                    }
                    i += len + 1;
                }

                if(found_aike) {
                    char mac_str[18];
                    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                             adv_report->address[5], adv_report->address[4],
                             adv_report->address[3], adv_report->address[2],
                             adv_report->address[1], adv_report->address[0]); // Reverse order usually for LE

                    furi_mutex_acquire(app->mutex, FuriWaitForever);

                    bool exists = false;
                    for(int j=0; j<app->device_count; j++) {
                        if(strcmp(app->device_macs[j], mac_str) == 0) {
                            exists = true;
                            break;
                        }
                    }

                    if(!exists && app->device_count < 10) {
                        strlcpy(app->device_names[app->device_count], name, sizeof(app->device_names[0]));
                        strlcpy(app->device_macs[app->device_count], mac_str, sizeof(app->device_macs[0]));
                        app->device_count++;

                        view_dispatcher_send_custom_event(app->view_dispatcher, AikeAgentCustomEventUpdateScan);
                    }

                    furi_mutex_release(app->mutex);
                }
            }
            // Return AckFlowEnable to let other handlers process it too if needed, 
            // but usually we are just observing.
            return BleEventNotAck;
        }
    }

    return BleEventNotAck;
}

static void aike_agent_start_scan(AikeAgentApp* app) {
    // Ensure stack is running
    if(!furi_hal_bt_is_active()) {
        furi_hal_bt_start_radio_stack();
    }

    // Register HCI event handler
    if(app->ble_event_handler == NULL) {
        app->ble_event_handler = ble_event_dispatcher_register_svc_handler(aike_hci_event_handler, app);
    }

    // Start General Discovery Procedure via ACI
    // Interval: 0x10 * 0.625ms = 10ms, Window: 0x10 * 0.625ms = 10ms (Continuous)
    // Own Address: Public (0x00)
    // Filter Duplicates: Enabled (0x01)
    tBleStatus status = aci_gap_start_general_discovery_proc(0x40, 0x30, OWN_ADDRESS_PUBLIC, DUPLICATE_FILTER_ENABLED);
    
    if (status != 0) {
        FURI_LOG_E(TAG, "Failed to start scan: 0x%02X", status);
    } else {
        FURI_LOG_I(TAG, "Scan started via ACI");
    }
}

static void aike_agent_stop_scan(AikeAgentApp* app) {
    if(app->ble_event_handler) {
        ble_event_dispatcher_unregister_svc_handler(app->ble_event_handler);
        app->ble_event_handler = NULL;
    }

    // Terminate GAP Procedure (0x02 = General Discovery)
    aci_gap_terminate_gap_proc(GAP_GENERAL_DISCOVERY_PROC);
}

// Event callbacks
static void aike_agent_scan_callback(void* context, uint32_t index) {
    AikeAgentApp* app = (AikeAgentApp*)context;
    aike_agent_stop_scan(app);

    char* mac_ptr = NULL;
    furi_mutex_acquire(app->mutex, FuriWaitForever);
    if(index < app->device_count) {
        mac_ptr = app->device_macs[index];
    }
    furi_mutex_release(app->mutex);

    if(mac_ptr) {
        view_dispatcher_switch_to_view(app->view_dispatcher, AikeAgentViewControl);
        app->current_view = AikeAgentViewControl;
        aike_agent_connect(app, mac_ptr);
    } else {
        aike_agent_start_scan(app);
    }
}

static void aike_agent_control_callback(void* context, AikeCommand command) {
    AikeAgentApp* app = (AikeAgentApp*)context;
    uint8_t cmd_buffer[10] = {0};

    switch(command) {
        case AikeCommandUnlock:
            cmd_buffer[1] = 0xD4; cmd_buffer[3] = 0x01;
            aike_agent_send_command(app, cmd_buffer, 10);
            break;
        case AikeCommandLock:
            cmd_buffer[1] = 0xD4; cmd_buffer[3] = 0x02;
            aike_agent_send_command(app, cmd_buffer, 10);
            break;
        case AikeCommandEcoOn:
            cmd_buffer[1] = 0xD4; cmd_buffer[3] = 0x03; cmd_buffer[4] = 0x01;
            aike_agent_send_command(app, cmd_buffer, 10);
            break;
        case AikeCommandEcoOff:
            cmd_buffer[1] = 0xD4; cmd_buffer[3] = 0x03;
            aike_agent_send_command(app, cmd_buffer, 10);
            break;
        case AikeCommandOpenBattery:
            cmd_buffer[1] = 0xD4; cmd_buffer[3] = 0x04;
            aike_agent_send_command(app, cmd_buffer, 10);
            break;
        case AikeCommandDisconnect:
            view_dispatcher_switch_to_view(app->view_dispatcher, AikeAgentViewScan);
            app->current_view = AikeAgentViewScan;
            aike_agent_start_scan(app);
            break;
        default: break;
    }
}

static void aike_agent_connect(AikeAgentApp* app, const char* mac_address) {
    // Parse MAC address from string "XX:XX:XX:XX:XX:XX"
    uint8_t addr[6];
    int values[6];
    if(6 == sscanf(mac_address, "%x:%x:%x:%x:%x:%x",
        &values[5], &values[4], &values[3], &values[2], &values[1], &values[0])) {
        for(int i=0; i<6; i++) addr[i] = (uint8_t)values[i];
    } else {
        FURI_LOG_E(TAG, "Invalid MAC format");
        return;
    }

    FURI_LOG_I(TAG, "Connecting to %02X:%02X:%02X:%02X:%02X:%02X",
        addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    // Update View
    aike_control_view_set_status(app->control_view, "Connecting...");

    // Start Connection
    // Scan: 0x10, 0x10 (Continuous fast scan)
    // Peer: Random(0x01) or Public(0x00)? Usually Random for scooters.
    // Try Random (0x01) first, or use detected type if we stored it.
    // For simplicity, assuming Random (0x01) if not Public.
    // Ideally we should pass the address type from scan result.

    // Using 0x00 (Public) for now as initial guess or based on scan?
    // In scan callback we don't pass type.
    // Let's assume 0x01 (Random) as most BLE peripherals use random static.
    // If it fails, we might need to change.

    tBleStatus status = aci_gap_create_connection(
        0x0010, 0x0010,
        0x01, addr, // Peer Addr Type: 0x01 (Random)
        OWN_ADDRESS_PUBLIC,
        0x0006, 0x0080, // 7.5ms - 100ms
        0,
        0x00C8, // 2000ms timeout
        0, 0
    );

    if(status != 0) {
        FURI_LOG_E(TAG, "Create Connection Failed: 0x%02X", status);
        aike_control_view_set_status(app->control_view, "Conn Cmd Failed");
    }
}

static void aike_agent_authenticate(AikeAgentApp* app) {
    UNUSED(MASTER_KEY);
    // Real Auth would require GATT Write
    FURI_LOG_I(TAG, "Authenticating... (GATT logic pending)");
    aike_control_view_set_status(app->control_view, "Connected (Auth Pending)");
}

static void aike_agent_send_command(AikeAgentApp* app, const uint8_t* command, uint8_t len) {
    UNUSED(app); UNUSED(command); UNUSED(len);
    FURI_LOG_I(TAG, "Command sent");
}

static bool aike_agent_view_dispatcher_custom_event_callback(void* context, uint32_t event) {
    AikeAgentApp* app = (AikeAgentApp*)context;
    if(event == AikeAgentCustomEventUpdateScan) {
        furi_mutex_acquire(app->mutex, FuriWaitForever);
        aike_scan_view_clear(app->scan_view);
        for(int i=0; i<app->device_count; i++) {
             aike_scan_view_add_item(app->scan_view, app->device_names[i], app->device_macs[i], i);
        }
        furi_mutex_release(app->mutex);
        return true;
    }
    return false;
}

static bool aike_agent_view_dispatcher_navigation_event_callback(void* context) {
    AikeAgentApp* app = (AikeAgentApp*)context;
    if(app->current_view == AikeAgentViewControl) {
        view_dispatcher_switch_to_view(app->view_dispatcher, AikeAgentViewScan);
        app->current_view = AikeAgentViewScan;
        aike_agent_start_scan(app);
        return true;
    }
    return false;
}

int32_t aike_agent_app(void* p) {
    UNUSED(p);
    AikeAgentApp* app = aike_agent_app_alloc();
    view_dispatcher_run(app->view_dispatcher);
    aike_agent_app_free(app);
    return 0;
}
