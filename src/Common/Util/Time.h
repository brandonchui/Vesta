#pragma once

#include "../Config.h"

namespace Time {
    VT_API void Initialize();

    VT_API void Tick();

    VT_API float GetDeltaTime();

    VT_API float GetTotalTime();
}
