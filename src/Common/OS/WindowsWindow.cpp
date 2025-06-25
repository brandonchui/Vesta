#include "../Config.h"

#if defined(TARGET_WINDOWS)

#include "../IApp.h"
#include "../IOperatingSystem.h"

#include <dwmapi.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>
#pragma comment(lib, "dwmapi.lib")

IApp* g_pApp = nullptr;
WindowDesc* g_pWindow = nullptr;
IInput* g_pInput = nullptr;

static WNDCLASSEXW gWindowClass = {};
static HWND gHWnd = NULL;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_pInput) {
        g_pInput->ProcessMessage(hWnd, uMsg, wParam, lParam);
    }

    if (!g_pApp || !g_pWindow) {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    switch (uMsg) {
        case WM_CREATE: {
            BOOL useDarkMode = TRUE;
            HRESULT hr = DwmSetWindowAttribute(hWnd, 20, &useDarkMode, sizeof(useDarkMode));
            if (FAILED(hr)) {
                DwmSetWindowAttribute(hWnd, 19, &useDarkMode, sizeof(useDarkMode));
            }
            SetWindowPos(hWnd,
                         NULL,
                         0,
                         0,
                         0,
                         0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            return 0;
        }
        case WM_CLOSE:
            g_pApp->mSettings.mQuit = true;
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            return 0;

        case WM_SIZE: {
            g_pApp->mSettings.mWidth = LOWORD(lParam);
            g_pApp->mSettings.mHeight = HIWORD(lParam);
            g_pWindow->width = LOWORD(lParam);
            g_pWindow->height = HIWORD(lParam);
            break;
        }

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool initWindowSystem() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!hInstance)
        return false;

    WCHAR wideTitle[128] = { 0 };
    MultiByteToWideChar(CP_UTF8, 0, g_pWindow->title, -1, wideTitle, 128);

    gWindowClass.cbSize = sizeof(WNDCLASSEXW);
    gWindowClass.style = CS_HREDRAW | CS_VREDRAW;
    gWindowClass.lpfnWndProc = WindowProc;
    gWindowClass.cbClsExtra = 0;
    gWindowClass.cbWndExtra = 0;
    gWindowClass.hInstance = hInstance;
    gWindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    gWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    gWindowClass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    gWindowClass.lpszMenuName = NULL;
    gWindowClass.lpszClassName = L"Application";
    gWindowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&gWindowClass)) {
        return false;
    }

    RECT windowRect = { 0, 0, g_pWindow->width, g_pWindow->height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    gHWnd = CreateWindowExW(0,
                            gWindowClass.lpszClassName,
                            wideTitle,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            windowRect.right - windowRect.left,
                            windowRect.bottom - windowRect.top,
                            NULL,
                            NULL,
                            hInstance,
                            NULL);

    if (!gHWnd) {
        return false;
    }

    g_pWindow->handle.window = gHWnd;
    g_pWindow->handle.type = WINDOW_HANDLE_TYPE_WINDOWS;

    ShowWindow(gHWnd, SW_SHOW);
    UpdateWindow(gHWnd);

    return true;
}

void exitWindowSystem() {
    if (gHWnd) {
        DestroyWindow(gHWnd);
    }
    UnregisterClassW(gWindowClass.lpszClassName, gWindowClass.hInstance);
}

bool handleMessages() {
    MSG msg = {};
    bool quit = false;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            quit = true;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return quit;
}

#endif // TARGET_WINDOWS
