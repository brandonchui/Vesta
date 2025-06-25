
#include "../Config.h"

#include <windows.h>
#include <windowsx.h>
// TODO move away from std
#include <bitset>
#include <optional>
#include <utility>
#include <vector>
#include "../IInput.h"

class WindowsInput final : public IInput {
  public:
    WindowsInput();
    ~WindowsInput();

    void NewFrameUpdate() override;
    void ProcessMessage(void* hWnd, uint32_t msg, uint64_t wParam, int64_t lParam) override;

    bool IsDown(Key k) const override;
    bool WasPressed(Key k) const override;
    bool WasReleased(Key k) const override;

    bool IsDown(MouseButton b) const override;
    bool WasPressed(MouseButton b) const override;
    bool WasReleased(MouseButton b) const override;

    bool IsComboDown(std::initializer_list<Key> keys) const override;
    bool WasComboPressed(std::initializer_list<Key> keys) const override;

    Vec2 GetMousePosition() const override;
    Vec2 GetMouseDelta() const override;

    void OnKeyDown(KeyCallback cb) override;
    void OnKeyUp(KeyCallback cb) override;
    void OnMouseDown(MouseCallback cb) override;
    void OnMouseUp(MouseCallback cb) override;
};

namespace {
    std::bitset<(size_t) Key::Count> g_keys;
    std::bitset<(size_t) Key::Count> g_prevKeys;
    std::bitset<(size_t) MouseButton::Count> g_mouseButtons;
    std::bitset<(size_t) MouseButton::Count> g_prevMouseButtons;

    Vec2 g_mousePos {};
    Vec2 g_prevMousePos {};
    Vec2 g_mouseDelta {};

    std::vector<KeyCallback> g_onKeyDownCBs;
    std::vector<KeyCallback> g_onKeyUpCBs;
    std::vector<MouseCallback> g_onMouseDownCBs;
    std::vector<MouseCallback> g_onMouseUpCBs;

    std::optional<Key> translate_vk_to_key(WPARAM wParam) {
        switch (wParam) {
            case VK_SHIFT:
                return Key::Shift;
            case VK_CONTROL:
                return Key::Ctrl;
            case VK_MENU:
                return Key::Alt;
            case 'A':
                return Key::A;
            case 'B':
                return Key::B;
            case 'C':
                return Key::C;
            case 'D':
                return Key::D;
            case 'E':
                return Key::E;
            case 'F':
                return Key::F;
            case 'G':
                return Key::G;
            case 'H':
                return Key::H;
            case 'I':
                return Key::I;
            case 'J':
                return Key::J;
            case 'K':
                return Key::K;
            case 'L':
                return Key::L;
            case 'M':
                return Key::M;
            case 'N':
                return Key::N;
            case 'O':
                return Key::O;
            case 'P':
                return Key::P;
            case 'Q':
                return Key::Q;
            case 'R':
                return Key::R;
            case 'S':
                return Key::S;
            case 'T':
                return Key::T;
            case 'U':
                return Key::U;
            case 'V':
                return Key::V;
            case 'W':
                return Key::W;
            case 'X':
                return Key::X;
            case 'Y':
                return Key::Y;
            case 'Z':
                return Key::Z;
            case '0':
                return Key::D0;
            case '1':
                return Key::D1;
            case '2':
                return Key::D2;
            case '3':
                return Key::D3;
            case '4':
                return Key::D4;
            case '5':
                return Key::D5;
            case '6':
                return Key::D6;
            case '7':
                return Key::D7;
            case '8':
                return Key::D8;
            case '9':
                return Key::D9;
            case VK_F1:
                return Key::F1;
            case VK_F2:
                return Key::F2;
            case VK_F3:
                return Key::F3;
            case VK_F4:
                return Key::F4;
            case VK_F5:
                return Key::F5;
            case VK_F6:
                return Key::F6;
            case VK_F7:
                return Key::F7;
            case VK_F8:
                return Key::F8;
            case VK_F9:
                return Key::F9;
            case VK_F10:
                return Key::F10;
            case VK_F11:
                return Key::F11;
            case VK_F12:
                return Key::F12;
            case VK_SPACE:
                return Key::Space;
            case VK_ESCAPE:
                return Key::Escape;
            case VK_TAB:
                return Key::Tab;
            case VK_RETURN:
                return Key::Return;
            default:
                return std::nullopt;
        }
    }

    void internal_NewFrameUpdate() {
        g_prevKeys = g_keys;
        g_prevMouseButtons = g_mouseButtons;
        g_prevMousePos = g_mousePos;
        g_mouseDelta = { g_mousePos.x - g_prevMousePos.x, g_mousePos.y - g_prevMousePos.y };
    }

    void internal_ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        Vec2 currentPos = { (float) GET_X_LPARAM(lParam), (float) GET_Y_LPARAM(lParam) };

        switch (msg) {
            case WM_KEYDOWN: {
                // Bit 30: The previous key state. 1 if down before, 0 if up.
                if (!(lParam & (1 << 30))) {
                    if (auto keyOpt = translate_vk_to_key(wParam)) {
                        Key k = *keyOpt;
                        for (auto& cb : g_onKeyDownCBs)
                            cb(k);
                        g_keys.set((size_t) k, true);
                    }
                }
                break;
            }
            case WM_KEYUP: {
                if (auto keyOpt = translate_vk_to_key(wParam)) {
                    Key k = *keyOpt;
                    for (auto& cb : g_onKeyUpCBs)
                        cb(k);
                    g_keys.set((size_t) k, false);
                }
                break;
            }
            case WM_MOUSEMOVE:
                g_mousePos = currentPos;
                break;
            case WM_LBUTTONDOWN:
                g_mousePos = currentPos;
                for (auto& cb : g_onMouseDownCBs)
                    cb(MouseButton::Left, g_mousePos);
                g_mouseButtons.set((size_t) MouseButton::Left, true);
                break;
            case WM_LBUTTONUP:
                g_mousePos = currentPos;
                for (auto& cb : g_onMouseUpCBs)
                    cb(MouseButton::Left, g_mousePos);
                g_mouseButtons.set((size_t) MouseButton::Left, false);
                break;
            case WM_RBUTTONDOWN:
                g_mousePos = currentPos;
                for (auto& cb : g_onMouseDownCBs)
                    cb(MouseButton::Right, g_mousePos);
                g_mouseButtons.set((size_t) MouseButton::Right, true);
                break;
            case WM_RBUTTONUP:
                g_mousePos = currentPos;
                for (auto& cb : g_onMouseUpCBs)
                    cb(MouseButton::Right, g_mousePos);
                g_mouseButtons.set((size_t) MouseButton::Right, false);
                break;
            case WM_MBUTTONDOWN:
                g_mousePos = currentPos;
                for (auto& cb : g_onMouseDownCBs)
                    cb(MouseButton::Middle, g_mousePos);
                g_mouseButtons.set((size_t) MouseButton::Middle, true);
                break;
            case WM_MBUTTONUP:
                g_mousePos = currentPos;
                for (auto& cb : g_onMouseUpCBs)
                    cb(MouseButton::Middle, g_mousePos);
                g_mouseButtons.set((size_t) MouseButton::Middle, false);
                break;
        }
    }
} // namespace

WindowsInput::WindowsInput() = default;
WindowsInput::~WindowsInput() = default;

void WindowsInput::NewFrameUpdate() { internal_NewFrameUpdate(); }

void WindowsInput::ProcessMessage(void*, uint32_t msg, uint64_t wParam, int64_t lParam) {
    internal_ProcessMessage((UINT) msg, (WPARAM) wParam, (LPARAM) lParam);
}

bool WindowsInput::IsDown(Key k) const { return g_keys.test((size_t) k); }
bool WindowsInput::WasPressed(Key k) const {
    return g_keys.test((size_t) k) && !g_prevKeys.test((size_t) k);
}
bool WindowsInput::WasReleased(Key k) const {
    return !g_keys.test((size_t) k) && g_prevKeys.test((size_t) k);
}

bool WindowsInput::IsDown(MouseButton b) const { return g_mouseButtons.test((size_t) b); }
bool WindowsInput::WasPressed(MouseButton b) const {
    return g_mouseButtons.test((size_t) b) && !g_prevMouseButtons.test((size_t) b);
}
bool WindowsInput::WasReleased(MouseButton b) const {
    return !g_mouseButtons.test((size_t) b) && g_prevMouseButtons.test((size_t) b);
}

bool WindowsInput::IsComboDown(std::initializer_list<Key> keys) const {
    for (auto k : keys)
        if (!IsDown(k))
            return false;
    return true;
}

bool WindowsInput::WasComboPressed(std::initializer_list<Key> keys) const {
    auto it = keys.begin();
    if (it == keys.end() || !WasPressed(*it))
        return false;
    for (++it; it != keys.end(); ++it)
        if (!IsDown(*it))
            return false;
    return true;
}

Vec2 WindowsInput::GetMousePosition() const { return g_mousePos; }
Vec2 WindowsInput::GetMouseDelta() const { return g_mouseDelta; }

void WindowsInput::OnKeyDown(KeyCallback cb) { g_onKeyDownCBs.push_back(std::move(cb)); }
void WindowsInput::OnKeyUp(KeyCallback cb) { g_onKeyUpCBs.push_back(std::move(cb)); }
void WindowsInput::OnMouseDown(MouseCallback cb) { g_onMouseDownCBs.push_back(std::move(cb)); }
void WindowsInput::OnMouseUp(MouseCallback cb) { g_onMouseUpCBs.push_back(std::move(cb)); }

IInput* CreateInputSystem() {
    static WindowsInput s_InputInstance;
    return &s_InputInstance;
}
