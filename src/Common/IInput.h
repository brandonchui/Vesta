#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include "Config.h"

// TODO use math lib
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

enum class Key : std::uint8_t {
    // Modifiers
    Shift,
    Ctrl,
    Alt,
    // Letters
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    // Numbers
    D0,
    D1,
    D2,
    D3,
    D4,
    D5,
    D6,
    D7,
    D8,
    D9,
    // F keys
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    // Misc
    Space,
    Escape,
    Tab,
    Return,
    // Count
    Count
};

enum class MouseButton : std::uint8_t {
    Left,
    Right,
    Middle,
    // Count
    Count
};

using KeyCallback = std::function<void(Key)>;
using MouseCallback = std::function<void(MouseButton, Vec2)>;

class VT_API IInput {
  public:
    virtual ~IInput() = default;

    virtual void NewFrameUpdate() = 0;
    virtual void ProcessMessage(void* hWnd, uint32_t msg, uint64_t wParam, int64_t lParam) = 0;

    // Pull
    virtual bool IsDown(Key k) const = 0;
    virtual bool WasPressed(Key k) const = 0;
    virtual bool WasReleased(Key k) const = 0;

    virtual bool IsDown(MouseButton b) const = 0;
    virtual bool WasPressed(MouseButton b) const = 0;
    virtual bool WasReleased(MouseButton b) const = 0;

    // BUG only works typed in sync from first to second key, not reverse
    virtual bool IsComboDown(std::initializer_list<Key> keys) const = 0;
    virtual bool WasComboPressed(std::initializer_list<Key> keys) const = 0;

    virtual Vec2 GetMousePosition() const = 0;
    virtual Vec2 GetMouseDelta() const = 0;

    // Push
    virtual void OnKeyDown(KeyCallback cb) = 0;
    virtual void OnKeyUp(KeyCallback cb) = 0;
    virtual void OnMouseDown(MouseCallback cb) = 0;
    virtual void OnMouseUp(MouseCallback cb) = 0;
};

VT_API IInput* CreateInputSystem();
