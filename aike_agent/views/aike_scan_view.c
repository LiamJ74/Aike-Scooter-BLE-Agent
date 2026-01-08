#include "aike_scan_view.h"
#include <gui/modules/submenu.h>
#include <stdlib.h>
#include <string.h>

struct AikeScanView {
    Submenu* submenu;
    AikeScanViewCallback callback;
    void* context;
};

static void aike_scan_view_submenu_callback(void* context, uint32_t index) {
    AikeScanView* instance = (AikeScanView*)context;
    if(instance->callback) {
        // Safe callback passing the index directly
        instance->callback(instance->context, index);
    }
}

AikeScanView* aike_scan_view_alloc() {
    AikeScanView* instance = malloc(sizeof(AikeScanView));
    instance->submenu = submenu_alloc();
    submenu_set_header(instance->submenu, "Scanning...");
    submenu_set_callback(instance->submenu, aike_scan_view_submenu_callback, instance);
    return instance;
}

void aike_scan_view_free(AikeScanView* instance) {
    submenu_free(instance->submenu);
    free(instance);
}

View* aike_scan_view_get_view(AikeScanView* instance) {
    return submenu_get_view(instance->submenu);
}

void aike_scan_view_add_item(AikeScanView* instance, const char* name, const char* mac, uint32_t index) {
    // Label format: "Name (MAC)"
    // Note: Flipper Submenu copies the label.
    // We use the provided index for the callback
    submenu_add_item(instance->submenu, name, index, aike_scan_view_submenu_callback, instance);
}

void aike_scan_view_set_callback(AikeScanView* instance, AikeScanViewCallback callback, void* context) {
    instance->callback = callback;
    instance->context = context;
}

void aike_scan_view_clear(AikeScanView* instance) {
    submenu_reset(instance->submenu);
    submenu_set_header(instance->submenu, "Scanning...");
}
