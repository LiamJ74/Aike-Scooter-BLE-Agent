#ifndef AIKE_SCAN_VIEW_H
#define AIKE_SCAN_VIEW_H

#include <gui/view.h>
#include <gui/modules/submenu.h>

typedef struct AikeScanView AikeScanView;

AikeScanView* aike_scan_view_alloc();
void aike_scan_view_free(AikeScanView* aike_scan_view);
View* aike_scan_view_get_view(AikeScanView* aike_scan_view);

// Function to add a device to the list
void aike_scan_view_add_item(AikeScanView* instance, const char* name, const char* mac, uint32_t index);

// Callback now passes the index
typedef void (*AikeScanViewCallback)(void* context, uint32_t index);

void aike_scan_view_set_callback(AikeScanView* instance, AikeScanViewCallback callback, void* context);
void aike_scan_view_clear(AikeScanView* instance);
void aike_scan_view_set_status(AikeScanView* instance, const char* status);

#endif
