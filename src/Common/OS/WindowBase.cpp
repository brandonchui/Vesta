#include <cstdio>

#include "../Config.h"
#include "../IApp.h"

#include "../IInput.h"
#include "../IOperatingSystem.h"
#include "../Util/Time.h"

static IApp* pApp = nullptr;
static WindowDesc gWindowDesc;

extern IApp* g_pApp;
extern WindowDesc* g_pWindow;
extern IInput* g_pInput;

extern bool initWindowSystem();
extern void exitWindowSystem();
extern bool handleMessages();

bool initBaseSubsystems() {
    // TODO
    return true;
}

void exitBaseSubsystems() {
    // TODO
}

//------------------------------------------------------------------------
// APP ENTRY POINT
//------------------------------------------------------------------------
int IApp::argc;
const char** IApp::argv;

int WindowsMain(int argc, char** argv, IApp* app) {
    IInput* pInputSystem = CreateInputSystem();

    pApp = app;

    g_pApp = pApp;
    g_pWindow = &gWindowDesc;
    g_pInput = pInputSystem;
    pApp->pInput = pInputSystem;

    IApp::Settings* pSettings = &pApp->mSettings;

    if (!pApp->Init()) {
        printf("Application initialization failed!\n");
        return EXIT_FAILURE;
    }

    gWindowDesc.width = pSettings->mWidth;
    gWindowDesc.height = pSettings->mHeight;
    gWindowDesc.title = pSettings->pTitle;

    if (!initWindowSystem()) {
        printf("Window system initialization failed!\n");
        pApp->ShutDown();
        return EXIT_FAILURE;
    }

    pApp->pWindow = g_pWindow;
    Time::Initialize();

    while (!pApp->mSettings.mQuit) {
        pInputSystem->NewFrameUpdate();
        if (handleMessages()) {
            pApp->mSettings.mQuit = true;
        }

        Time::Tick();
        pApp->Update(Time::GetDeltaTime());
        pApp->Draw();
    }

    pApp->ShutDown();
    exitWindowSystem();

    return EXIT_SUCCESS;
}
