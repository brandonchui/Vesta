#include <cstdint>
#include "Time.h"

#if defined(TARGET_WINDOWS)

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#elif defined(TARGET_MACOS)
// TODO: Add macOS includes
#error "macOS support not yet implemented"
#else
#error "Unsupported platform"
#endif

namespace {
    struct TimerState {
        float secondsPerCount = 0.0;
        float deltaTime = -1.0;
        int64_t baseTime = 0;
        int64_t prevTime = 0;
        int64_t currentTime = 0;
    };

    TimerState g_State;
}

namespace Time {
    void Initialize() {
#if defined(TARGET_WINDOWS)
        int64_t countsPerSecond = 0;

        QueryPerformanceFrequency((LARGE_INTEGER*) &countsPerSecond);
        g_State.secondsPerCount = 1.0 / static_cast<float>(countsPerSecond);

        // Get the current time as starting point
        QueryPerformanceCounter((LARGE_INTEGER*) &g_State.baseTime);
        g_State.currentTime = g_State.baseTime;
        g_State.prevTime = g_State.baseTime;
        g_State.deltaTime = 0.0;
#elif defined(TARGET_MACOS)
// TODO: Implement macOS timer initialization
#error "macOS timer initialization not yet implemented"
#endif
    }

    void Tick() {
#if defined(TARGET_WINDOWS)
        QueryPerformanceCounter((LARGE_INTEGER*) &g_State.currentTime);

        // Frame to frame time delta
        if (g_State.currentTime < g_State.prevTime) {
            g_State.deltaTime = 0.0;
        } else {
            g_State.deltaTime = (g_State.currentTime - g_State.prevTime) * g_State.secondsPerCount;
        }

        g_State.prevTime = g_State.currentTime;
#elif defined(TARGET_MACOS)
// TODO: Implement macOS timer tick
#error "macOS timer tick not yet implemented"
#endif
    }

    float GetDeltaTime() { return static_cast<float>(g_State.deltaTime); }

    float GetTotalTime() {
        return static_cast<float>((g_State.currentTime - g_State.baseTime) *
                                  g_State.secondsPerCount);
    }
} // end namespace Time
