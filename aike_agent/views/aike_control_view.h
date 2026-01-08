#ifndef AIKE_CONTROL_VIEW_H
#define AIKE_CONTROL_VIEW_H

#include <gui/view.h>
#include <gui/modules/submenu.h>

typedef struct AikeControlView AikeControlView;

typedef enum {
    AikeCommandUnlock,
    AikeCommandLock,
    AikeCommandEcoOn,
    AikeCommandEcoOff,
    AikeCommandOpenBattery,
    AikeCommandDisconnect
} AikeCommand;

typedef void (*AikeControlViewCallback)(void* context, AikeCommand command);

AikeControlView* aike_control_view_alloc();
void aike_control_view_free(AikeControlView* instance);
View* aike_control_view_get_view(AikeControlView* instance);
void aike_control_view_set_callback(AikeControlView* instance, AikeControlViewCallback callback, void* context);
void aike_control_view_set_status(AikeControlView* instance, const char* status);

#endif
