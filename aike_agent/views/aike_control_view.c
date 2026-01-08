#include "aike_control_view.h"
#include <gui/modules/submenu.h>
#include <stdlib.h>

struct AikeControlView {
    Submenu* submenu;
    AikeControlViewCallback callback;
    void* context;
};

static void aike_control_view_submenu_callback(void* context, uint32_t index) {
    AikeControlView* instance = (AikeControlView*)context;
    if(instance->callback) {
        instance->callback(instance->context, (AikeCommand)index);
    }
}

AikeControlView* aike_control_view_alloc() {
    AikeControlView* instance = malloc(sizeof(AikeControlView));
    instance->submenu = submenu_alloc();
    submenu_set_header(instance->submenu, "Aike Controls");

    // The index must match the enum AikeCommand order
    submenu_add_item(instance->submenu, "Unlock", AikeCommandUnlock, aike_control_view_submenu_callback, instance);
    submenu_add_item(instance->submenu, "Lock", AikeCommandLock, aike_control_view_submenu_callback, instance);
    submenu_add_item(instance->submenu, "Eco Mode ON", AikeCommandEcoOn, aike_control_view_submenu_callback, instance);
    submenu_add_item(instance->submenu, "Eco Mode OFF", AikeCommandEcoOff, aike_control_view_submenu_callback, instance);
    submenu_add_item(instance->submenu, "Open Battery", AikeCommandOpenBattery, aike_control_view_submenu_callback, instance);
    submenu_add_item(instance->submenu, "Disconnect", AikeCommandDisconnect, aike_control_view_submenu_callback, instance);

    return instance;
}

void aike_control_view_free(AikeControlView* instance) {
    submenu_free(instance->submenu);
    free(instance);
}

View* aike_control_view_get_view(AikeControlView* instance) {
    return submenu_get_view(instance->submenu);
}

void aike_control_view_set_callback(AikeControlView* instance, AikeControlViewCallback callback, void* context) {
    instance->callback = callback;
    instance->context = context;
}

void aike_control_view_set_status(AikeControlView* instance, const char* status) {
    submenu_set_header(instance->submenu, status);
}
