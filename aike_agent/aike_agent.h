#ifndef AIKE_AGENT_H
#define AIKE_AGENT_H

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/variable_item_list.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#include "aike_agent_icons.h"
#include "views/aike_scan_view.h"
#include "views/aike_control_view.h"

typedef enum {
    AikeAgentViewScan,
    AikeAgentViewControl,
    AikeAgentViewNone,
} AikeAgentView;

typedef struct {
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    NotificationApp* notifications;
    Submenu* submenu;

    // Custom views
    AikeScanView* scan_view;
    AikeControlView* control_view;

    // Application state
    FuriMutex* mutex;

    // Devices list for safe storage
    char device_names[10][32];
    char device_macs[10][18];
    uint8_t device_count;

    AikeAgentView current_view;
} AikeAgentApp;

typedef enum {
    AikeAgentCustomEventUpdateScan = 100,
} AikeAgentCustomEvent;

#endif
