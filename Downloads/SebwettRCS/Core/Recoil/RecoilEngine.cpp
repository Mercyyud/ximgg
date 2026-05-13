#include "RecoilEngine.h"
#include "../Input/Mouse.h"

#include <Windows.h>
#include <atomic>
#include <cmath>
#include <thread>

namespace RecoilEngine {

static std::atomic<bool>  s_Enabled  { false };
static std::atomic<bool>  s_Running  { false };
static std::atomic<bool>  s_StopFlag { false };

static std::atomic<float> s_RecoilX  { 0.0f  };
static std::atomic<float> s_RecoilY  { 20.0f };
static std::atomic<bool>  s_OnlyADS  { true  };
static std::atomic<bool>  s_OnlyR6   { true  };
static std::atomic<int>   s_RecoilHotkey { 0 };

// Returns true when Rainbow Six Siege (or our own overlay) is in the foreground
static bool IsR6Active() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;

    wchar_t title[256] = {};
    GetWindowTextW(fg, title, 256);

    if (wcsstr(title, L"Rainbow Six") != nullptr) return true;

    // Our overlay is on top — check that R6 is still running behind it
    if (wcsstr(title, L"xim.gg") != nullptr) {
        HWND r6 = FindWindowW(NULL, L"Rainbow Six");
        return r6 != NULL && IsWindowVisible(r6);
    }
    return false;
}

static void RecoilThread() {
    s_Running = true;

    while (!s_StopFlag) {
        // Handle hotkey toggle
        static bool s_PrevHotkeyState = false;
        int hotkey = s_RecoilHotkey.load();
        if (hotkey != 0) {
            bool currentState = (GetAsyncKeyState(hotkey) & 0x8000) != 0;
            if (currentState && !s_PrevHotkeyState) {
                s_Enabled = !s_Enabled.load();
            }
            s_PrevHotkeyState = currentState;
        }

        if (!s_Enabled.load()) { Sleep(10); continue; }
        if (s_OnlyR6.load() && !IsR6Active()) { Sleep(10); continue; }

        bool ads     = true;
        bool firing  = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (s_OnlyADS.load())
            ads = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

        if (firing && ads) {
            float rx = s_RecoilX.load();
            float ry = s_RecoilY.load();

            if (rx != 0.0f || ry != 0.0f)
                Mouse::Move((int)roundf(rx), (int)roundf(ry));

            Sleep(10); // ~100 Hz compensation rate
        } else {
            Sleep(1);
        }
    }

    s_Running = false;
}

void Start() {
    if (s_Running.load()) return;
    s_StopFlag = false;
    std::thread(RecoilThread).detach();
}

void Stop() {
    s_StopFlag = true;
    for (int i = 0; i < 100 && s_Running.load(); ++i)
        Sleep(10);
}

void SetEnabled(bool enabled) { s_Enabled = enabled; }
bool IsEnabled()              { return s_Enabled.load(); }

void SetParams(float recoilX, float recoilY, bool onlyADS, bool onlyR6) {
    s_RecoilX  = recoilX;
    s_RecoilY  = recoilY;
    s_OnlyADS  = onlyADS;
    s_OnlyR6   = onlyR6;
}
void SetHotkey(int vk) { s_RecoilHotkey = vk; }

} // namespace RecoilEngine
