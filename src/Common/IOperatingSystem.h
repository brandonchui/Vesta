#pragma once

#include <wtypes.h>
#include "Config.h"

typedef enum WindowHandleType WindowHandleType;
enum WindowHandleType {
    WINDOW_HANDLE_TYPE_UNKNOWN,
    WINDOW_HANDLE_TYPE_WINDOWS,
};

typedef struct WindowHandle WindowHandle;
struct WindowHandle {
    WindowHandleType type = WINDOW_HANDLE_TYPE_UNKNOWN;
#if defined(TARGET_WINDOWS)
    HWND window = NULL;
#endif
};

struct WindowDesc {
    WindowHandle handle;
    int32_t width = 1280;
    int32_t height = 720;
    const char* title = "My Application";
};
