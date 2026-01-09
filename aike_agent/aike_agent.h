#ifndef AIKE_AGENT_H
#define AIKE_AGENT_H

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/variable_item_list.h>
#include "views/aike_scan_view.h"
#include "views/aike_control_view.h"
#include "aike_agent_stubs.h" // Needed for GapSvcEventHandler definition

typedef enum {
    AikeAgentViewScan,
    AikeAgentViewControl,
} AikeAgentView;

typedef enum {
    AikeAgentCustomEventUpdateScan,
} AikeAgentCustomEvent;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    AikeScanView* scan_view;
    AikeControlView* control_view;
    FuriMutex* mutex;
    
    char device_names[10][32];
    char device_macs[10][18];
    uint8_t device_count;
    
    AikeAgentView current_view;
    GapSvcEventHandler* ble_event_handler;
} AikeAgentApp;

#endif
