#include "Application.h"

#include "../Config/Config.h"
#include "../Input/Mouse.h"
#include "../Recoil/RecoilEngine.h"
#include "../../Rendering/Overlay/Overlay.h"
#include "../../Rendering/Menu/Menu.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <dwmapi.h>
#include <wininet.h>
#include <string>
#include <sstream>

#pragma comment(lib, "wininet.lib")

// Forward-declare ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Application {

static bool g_Running    = true;
static bool g_Authed     = false;   // set to true after successful auth
static bool g_AuthFailed = false;   // set to true on bad key
static char g_KeyBuf[64] = {};      // license key input buffer
static char g_StatusMsg[128] = {};  // status message shown under input

// ── HTTP POST helper (WinINet, no extra libs) ─────────────────────────────
static std::string HttpPost(const std::string& host, int port,
                             const std::string& path, const std::string& body)
{
    std::string result;
    HINTERNET hSession = InternetOpenA("xim.gg", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return result;

    HINTERNET hConnect = InternetConnectA(hSession, host.c_str(), (INTERNET_PORT)port,
                                           NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hSession); return result; }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (port == 443) flags |= INTERNET_FLAG_SECURE;

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path.c_str(),
                                           NULL, NULL, NULL, flags, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return result;
    }

    std::string headers = "Content-Type: application/json\r\n";
    HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.size(),
                     (LPVOID)body.c_str(), (DWORD)body.size());

    char buf[2048] = {};
    DWORD read = 0;
    while (InternetReadFile(hRequest, buf, sizeof(buf) - 1, &read) && read > 0) {
        result.append(buf, read);
        read = 0;
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);
    return result;
}

// ── Validate key against the xim.gg API ──────────────────────────────────
// Change API_HOST / API_PORT / GUILD_ID to match your Railway deployment
static constexpr const char* API_HOST  = "web-production-7be10.up.railway.app";
static constexpr int         API_PORT  = 443;
static constexpr const char* GUILD_ID  = "1419533443812950088";

static bool ValidateKey(const std::string& key)
{
    // Build JSON body
    std::string body = "{\"guild_id\":" + std::string(GUILD_ID) +
                       ",\"license_key\":\"" + key + "\"}";

    std::string resp = HttpPost(API_HOST, API_PORT, "/api/log_login", body);

    // Check for success
    return resp.find("\"status\":\"success\"") != std::string::npos ||
           resp.find("\"status\": \"success\"") != std::string::npos;
}

// ── Auth screen (rendered every frame until g_Authed) ────────────────────
static void RenderAuthScreen()
{
    ImGuiIO& io = ImGui::GetIO();

    // Full-screen dark backdrop
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::Begin("##auth_bg", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Centered card
    float cardW = 340.0f, cardH = 210.0f;
    float cx = (io.DisplaySize.x - cardW) * 0.5f;
    float cy = (io.DisplaySize.y - cardH) * 0.5f;

    // Card background
    dl->AddRectFilled({ cx, cy }, { cx + cardW, cy + cardH },
        IM_COL32(14, 18, 26, 255), 10.0f);
    dl->AddRect({ cx, cy }, { cx + cardW, cy + cardH },
        IM_COL32(58, 171, 249, 180), 10.0f, 0, 1.5f);

    // Top accent bar
    dl->AddRectFilled({ cx, cy }, { cx + cardW, cy + 3.0f },
        IM_COL32(58, 171, 249, 255), 10.0f);

    // Title
    const char* title = "xim.gg";
    ImVec2 titleSz = ImGui::CalcTextSize(title);
    ImGui::SetWindowFontScale(1.4f);
    dl->AddText({ cx + (cardW - titleSz.x * 1.4f) * 0.5f, cy + 18.0f },
        IM_COL32(255, 255, 255, 255), title);
    ImGui::SetWindowFontScale(1.0f);

    // Subtitle
    const char* sub = "Enter your license key";
    ImVec2 subSz = ImGui::CalcTextSize(sub);
    dl->AddText({ cx + (cardW - subSz.x) * 0.5f, cy + 52.0f },
        IM_COL32(140, 160, 190, 255), sub);

    // Input field
    ImGui::SetCursorScreenPos({ cx + 20.0f, cy + 80.0f });
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        IM_COL32(22, 28, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(28, 36, 52, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  IM_COL32(30, 40, 58, 255));
    ImGui::PushStyleColor(ImGuiCol_Border,         IM_COL32(58, 171, 249, 120));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  { 10.0f, 8.0f });
    ImGui::SetNextItemWidth(cardW - 40.0f);
    ImGui::InputText("##key", g_KeyBuf, sizeof(g_KeyBuf));
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    // Authenticate button
    ImGui::SetCursorScreenPos({ cx + 20.0f, cy + 122.0f });
    ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(58, 171, 249, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(40, 140, 210, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(30, 110, 180, 255));
    ImGui::PushStyleColor(ImGuiCol_Text,          IM_COL32(10, 14, 20, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    bool enter = ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                 ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);

    if (ImGui::Button("Authenticate", { cardW - 40.0f, 34.0f }) || enter) {
        std::string key(g_KeyBuf);
        if (key.empty()) {
            strcpy_s(g_StatusMsg, "Please enter a license key.");
            g_AuthFailed = true;
        } else {
            strcpy_s(g_StatusMsg, "Verifying...");
            g_AuthFailed = false;

            if (ValidateKey(key)) {
                g_Authed = true;
                g_StatusMsg[0] = '\0';
            } else {
                strcpy_s(g_StatusMsg, "Invalid or expired key.");
                g_AuthFailed = true;
            }
        }
    }

    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(4);

    // Status message
    if (g_StatusMsg[0] != '\0') {
        ImVec2 msgSz = ImGui::CalcTextSize(g_StatusMsg);
        ImU32  msgCol = g_AuthFailed ? IM_COL32(220, 80, 80, 255)
                                     : IM_COL32(58, 171, 249, 255);
        dl->AddText({ cx + (cardW - msgSz.x) * 0.5f, cy + 168.0f }, msgCol, g_StatusMsg);
    }

    ImGui::End();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_NCHITTEST: return HTCLIENT;
    case WM_SIZE:      return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        g_Running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int Run(HINSTANCE hInstance) {
    // ── Window ────────────────────────────────────────────────────────
    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"XimGGOverlay";
    RegisterClassExW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_APPWINDOW,
        wc.lpszClassName, L"xim.gg",
        WS_POPUP, 0, 0, sw, sh,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"Failed to create overlay window.", L"xim.gg", MB_ICONERROR);
        return 1;
    }

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    MARGINS margin = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margin);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd);

    // ── Subsystems ────────────────────────────────────────────────────
    if (!Overlay::Initialize(hwnd)) {
        MessageBoxW(NULL, L"Failed to initialise DirectX 11.", L"xim.gg", MB_ICONERROR);
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }

    Mouse::Open();

    Config::Settings settings = Config::LoadSettings();

    // Show overlay immediately for auth screen
    Overlay::SetVisible(true);

    // Remove click-through so user can type in the auth screen
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
    SetForegroundWindow(hwnd);

    // ── Message + render loop ─────────────────────────────────────────
    static bool s_MenuKeyWasDown = false;
    static bool s_MenuInitialized = false;

    MSG msg = {};
    while (g_Running) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) g_Running = false;
        }
        if (!g_Running) break;

        if (!g_Authed) {
            // Show auth screen
            Overlay::BeginFrame();
            RenderAuthScreen();
            Overlay::EndFrame();
            continue;
        }

        // First frame after auth — initialize menu and recoil
        if (!s_MenuInitialized) {
            Menu::Initialize();
            RecoilEngine::Start();
            // Restore click-through for the overlay
            LONG_PTR es = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, es | WS_EX_TRANSPARENT);
            Overlay::SetVisible(false); // hide until INSERT
            s_MenuInitialized = true;
        }

        // INSERT toggles the menu
        bool menuKeyDown = (GetAsyncKeyState(settings.menuHotkey) & 0x8000) != 0;
        if (menuKeyDown && !s_MenuKeyWasDown) {
            HWND fg = GetForegroundWindow();
            wchar_t title[256] = {};
            if (fg) GetWindowTextW(fg, title, 256);
            bool r6Active      = wcsstr(title, L"Rainbow Six")  != nullptr;
            bool overlayActive = wcsstr(title, L"XimGGOverlay") != nullptr;

            if (r6Active || overlayActive)
                Overlay::SetVisible(!Overlay::IsVisible());
        }
        s_MenuKeyWasDown = menuKeyDown;

        // Render frame
        Overlay::BeginFrame();
        if (Overlay::IsVisible())
            Menu::Render();
        Overlay::EndFrame();
    }

    // ── Cleanup ───────────────────────────────────────────────────────
    if (s_MenuInitialized) {
        RecoilEngine::Stop();
    }
    Mouse::Close();
    Overlay::Shutdown();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);
    return 0;
}

} // namespace Application
