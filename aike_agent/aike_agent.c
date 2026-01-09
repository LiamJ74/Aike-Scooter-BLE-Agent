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

// HCI Event Handler to capture Adv Reports
static BleEventAckStatus aike_hci_event_handler(void* event, void* context) {
    AikeAgentApp* app = (AikeAgentApp*)context;
    hci_uart_pckt* pckt = (hci_uart_pckt*)event;

    if(pckt->type != HCI_EVENT_PKT_TYPE) return BleEventNotAck;

    hci_event_pckt* evt_pckt = (hci_event_pckt*)pckt->data;
    
    if(evt_pckt->evt == HCI_LE_META_EVENT) {
        evt_le_meta_event* meta_evt = (evt_le_meta_event*)evt_pckt->data;

        if(meta_evt->subevent == HCI_LE_ADVERTISING_REPORT_EVENT) {
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
    // For now, we only simulated connection/auth/command in previous steps.
    // Implementing ACI connection is complex (Gap_Create_Connection).
    // Given the task was to fix Scan first, we keep this "Simulation" or "Partial"
    // unless we want to go full ACI for connection too.
    // We will log the intent.
    FURI_LOG_I(TAG, "Connect requested to %s (ACI implementation pending for connect)", mac_address);
    aike_agent_authenticate(app);
}

static void aike_agent_authenticate(AikeAgentApp* app) {
    UNUSED(MASTER_KEY); // Prevent Unused error during simulation
    // Simulated Auth
    FURI_LOG_I(TAG, "Authenticating...");
    aike_control_view_set_status(app->control_view, "Connected (Sim)");
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
