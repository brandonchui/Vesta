#pragma once

#include "Config.h"
#include "IInput.h"
#include "IOperatingSystem.h"

class VT_API IApp {
  public:
    virtual ~IApp() = default;

    virtual bool Init() = 0;
    virtual void ShutDown() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;

    // Getter
    virtual const char* GetName() = 0;

  public:
    struct Settings {
        int32_t mWidth = 1280;
        int32_t mHeight = 720;
        const char* pTitle = "Default App";
        bool mQuit = false;
    } mSettings;

    WindowDesc* pWindow = nullptr;
    IInput* pInput = nullptr;

    // Command line args
    static int argc;
    static const char** argv;
};

// Entry
#if defined(TARGET_WINDOWS)
VT_API extern int WindowsMain(int argc, char** argv, IApp* app);

#define DEFINE_APPLICATION_MAIN(appClass)                                                          \
    int main(int argc, char** argv) {                                                              \
        IApp::argc = argc;                                                                         \
        IApp::argv = (const char**) argv;                                                          \
        static appClass app = {};                                                                  \
        return WindowsMain(argc, argv, &app);                                                      \
    }
#elif defined(TARGET_MACOS)
// TODO: Implement macOS entry point
#define DEFINE_APPLICATION_MAIN(appClass)                                                          \
    int main(int argc, char** argv) { return 0; }
#endif
