#include "Application.h"
#include "../Config/Config.h"
#include "../Input/Mouse.h"
#include "../Recoil/RecoilEngine.h"
#include "../../Rendering/Overlay/Overlay.h"
#include "../../Rendering/Menu/Menu.h"
#include "../AI/AIModule.h"
#include "../Security/Security.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <dwmapi.h>
#include <wininet.h>
#include <d3d11.h>
#include <shellapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "wininet.lib")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace player_img {
#include "player.c"
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Application {

class AppInstance {
public:
    AppInstance() = default;
    ~AppInstance() { Cleanup(); }

    bool Initialize(HINSTANCE hInst);
    void Run();
    void Cleanup();

    void RenderFrame();
    void HandleAuthTransition();
    
    bool m_Running = true;
    bool m_Authed = false;
    bool m_AuthFailed = false;
    bool m_MenuKeyWasDown = false;
    bool m_EngineInitialized = false;

    HWND m_Hwnd = NULL;
    char m_KeyBuf[64] = {};
    char m_StatusMsg[128] = {};

private:
    void RenderAuthScreen();
    bool ValidateKey(const std::string& key);
    std::string HttpPost(const std::string& host, int port, const std::string& path, const std::string& body);
    std::string GetHWID();
    void EnsureAssets();
};

static AppInstance* g_App = nullptr;

static constexpr const char* API_HOST = "web-production-7be10.up.railway.app";
static constexpr int API_PORT = 443;
static constexpr const char* GUILD_ID = "1419533443812950088";

std::string AppInstance::GetHWID() {
    std::string sid;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
        wchar_t buf[256] = {};
        DWORD sz = sizeof(buf);
        if (RegQueryValueExW(hKey, L"MachineGuid", NULL, NULL, (BYTE*)buf, &sz) == ERROR_SUCCESS) {
            int n = WideCharToMultiByte(CP_UTF8, 0, buf, -1, NULL, 0, NULL, NULL);
            sid.resize(n - 1);
            WideCharToMultiByte(CP_UTF8, 0, buf, -1, &sid[0], n, NULL, NULL);
        }
        RegCloseKey(hKey);
    }
    return sid;
}

std::string AppInstance::HttpPost(const std::string& host, int port, const std::string& path, const std::string& body) {
    std::string result;
    HINTERNET hSession = InternetOpenA("xim.gg", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return result;
    HINTERNET hConnect = InternetConnectA(hSession, host.c_str(), (INTERNET_PORT)port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hSession); return result; }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (port == 443) flags |= INTERNET_FLAG_SECURE;

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path.c_str(), NULL, NULL, NULL, flags, 0);
    if (hRequest) {
        std::string headers = "Content-Type: application/json\r\n";
        HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.size(), (LPVOID)body.c_str(), (DWORD)body.size());
        char buf[2048];
        DWORD read = 0;
        while (InternetReadFile(hRequest, buf, sizeof(buf) - 1, &read) && read > 0) {
            result.append(buf, read);
        }
        InternetCloseHandle(hRequest);
    }
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);
    return result;
}

bool AppInstance::ValidateKey(const std::string& key) {
    std::string body = "{\"guild_id\":" + std::string(GUILD_ID) + ",\"license_key\":\"" + key + "\",\"hwid\":\"" + GetHWID() + "\"}";
    std::string resp = HttpPost(API_HOST, API_PORT, "/api/log_login", body);
    return resp.find("\"status\":\"success\"") != std::string::npos || resp.find("\"status\": \"success\"") != std::string::npos;
}

void AppInstance::EnsureAssets() {
    // Assets already embedded
}

void AppInstance::RenderAuthScreen() {
    ImGuiIO& io = ImGui::GetIO();
    float sw = 850.f, sh = 640.f;

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize({ sw, sh });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
    ImGui::Begin("##auth", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar(2);

    auto dl = ImGui::GetWindowDrawList();
    ImVec2 cp = ImGui::GetWindowPos();

    dl->AddRectFilled(cp, { cp.x + sw, cp.y + sh }, IM_COL32(14, 14, 14, 255), 12.f);
    dl->AddRect({ cp.x, cp.y }, { cp.x + sw, cp.y + sh }, IM_COL32(35, 35, 35, 255), 12.f, 0, 1.5f);

    float hH = 45.f;
    dl->AddLine({ cp.x, cp.y + hH }, { cp.x + sw, cp.y + hH }, IM_COL32(255, 255, 255, 10));

    ImGui::SetCursorPos({ 25, 14 });
    ImGui::TextDisabled("SIGNUP /"); ImGui::SameLine();
    ImGui::TextColored(ImColor(220, 220, 220), "LOGIN");

    ImGui::SetCursorPos({ sw - 35, 12 });
    if (ImAdd::ButtonXMark("exit", { 22, 22 })) PostQuitMessage(0);

    float rx = (sw) * 0.5f, ry = hH + 100.f;
    ImGui::PushFont(io.Fonts->Fonts[1]);
    ImGui::SetCursorPos({ rx - ImGui::CalcTextSize("XIM RECOIL").x * 0.5f, ry });
    ImGui::Text("XIM RECOIL");
    ImGui::PopFont();

    ry += 30.f;
    ImGui::SetCursorPos({ rx - ImGui::CalcTextSize(".gg/ximgg").x * 0.5f, ry });
    ImGui::TextColored(ImColor(160, 100, 255, 220), ".gg/ximgg");

    ry += 60.f;
    ImGui::SetCursorPos({ rx - 110, ry }); ImGui::TextDisabled("license key");
    ry += 25.f;
    ImGui::SetCursorPos({ rx - 110, ry });
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(24, 24, 24, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 12, 12 });
    ImGui::SetNextItemWidth(220);
    ImGui::InputText("##key", m_KeyBuf, sizeof(m_KeyBuf));
    ImGui::PopStyleVar(2); ImGui::PopStyleColor();

    ry += 75.f;
    ImGui::SetCursorPos({ rx - 110, ry });
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(150, 80, 250, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    if (ImGui::Button("LOGIN", { 220, 44 }) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        if (m_KeyBuf[0] != '\0' && ValidateKey(m_KeyBuf)) {
            m_Authed = true;
            auto s = Config::LoadSettings();
            s.licenseKey = m_KeyBuf;
            Config::SaveSettings(s);
            HandleAuthTransition();
        } else {
            strcpy_s(m_StatusMsg, "Invalid credentials.");
            m_AuthFailed = true;
        }
    }
    ImGui::PopStyleVar(); ImGui::PopStyleColor();

    if (m_AuthFailed) {
        ImGui::SetCursorPos({ rx - 110, ry + 55 });
        ImGui::TextColored(ImColor(255, 50, 50), m_StatusMsg);
    }

    ImGui::SetCursorPos({ rx - ImGui::CalcTextSize("open a ticket").x * 0.5f, sh - 40 });
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 80, 250, 200));
    if (ImGui::Selectable("open a ticket", false, 0, ImGui::CalcTextSize("open a ticket"))) ShellExecuteA(0, "open", "https://discord.gg/ximgg", 0, 0, SW_SHOWNORMAL);
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImGui::PopStyleColor();

    ImGui::End();
}

void AppInstance::HandleAuthTransition() {
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    
    SetWindowPos(m_Hwnd, HWND_TOPMOST, 0, 0, sw, sh, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    Overlay::Resize(sw, sh);
    
    LONG_PTR es = GetWindowLongPtrW(m_Hwnd, GWL_EXSTYLE);
    SetWindowLongPtrW(m_Hwnd, GWL_EXSTYLE, es | WS_EX_TRANSPARENT | WS_EX_LAYERED);
    
    Overlay::SetStreamProof(Menu::GetSettings().streamProof);
    Overlay::SetVisible(false, false);
    RecoilEngine::Start();
    m_EngineInitialized = true;
}

bool AppInstance::Initialize(HINSTANCE hInst) {
    Security::Initialize();
    WNDCLASSEXW wc = { sizeof(wc), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst, 0, LoadCursor(0, IDC_ARROW), 0, 0, L"XimGG", 0 };
    RegisterClassExW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    int aw = 850, ah = 640;
    
    m_Hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_APPWINDOW, wc.lpszClassName, L"xim.gg", WS_POPUP, (sw - aw) / 2, (sh - ah) / 2, aw, ah, 0, 0, hInst, 0);
    if (!m_Hwnd) return false;

    SetLayeredWindowAttributes(m_Hwnd, 0, 255, LWA_ALPHA);
    MARGINS m = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(m_Hwnd, &m);
    
    if (!Overlay::Initialize(m_Hwnd)) return false;
    
    Mouse::Open();
    auto s = Config::LoadSettings();
    if (s.rememberKey && !s.licenseKey.empty()) strcpy_s(m_KeyBuf, s.licenseKey.c_str());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    Menu::Initialize(m_Hwnd, Overlay::GetDevice(), Overlay::GetContext());
    AIModule::Initialize();
    Overlay::SetStreamProof(false);
    Overlay::SetVisible(true, true);
    
    ShowWindow(m_Hwnd, SW_SHOW);
    UpdateWindow(m_Hwnd);
    SetForegroundWindow(m_Hwnd);
    return true;
}

void AppInstance::RenderFrame() {
    Overlay::BeginFrame();
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (!m_Authed) {
        RenderAuthScreen();
    } else {
        auto& settings = Menu::GetSettings();
        if (settings.menuHotkey == 0) settings.menuHotkey = VK_INSERT;

        bool menuDown = (GetAsyncKeyState(settings.menuHotkey) & 0x8000) != 0;
        if (menuDown && !m_MenuKeyWasDown) {
            Overlay::SetVisible(!Overlay::IsVisible(), true);
            if (Overlay::IsVisible()) {
                SetForegroundWindow(m_Hwnd);
                ImGui::SetNextWindowFocus();
            }
        }
        m_MenuKeyWasDown = menuDown;

        if (Overlay::IsVisible()) {
            Menu::RenderInternal();
        }
    }

    ImGui::Render();
    Overlay::EndFrame();
    auto drawData = ImGui::GetDrawData();
    if (drawData) ImGui_ImplDX11_RenderDrawData(drawData);
    Overlay::Present();
}

void AppInstance::Run() {
    MSG msg = {};
    while (m_Running) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) m_Running = false;
        }
        RenderFrame();
    }
}

void AppInstance::Cleanup() {
    if (m_EngineInitialized) RecoilEngine::Stop();
    Mouse::Close();
    Overlay::Shutdown();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (Menu::HandleMessage(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_NCHITTEST: return HTCLIENT;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int Run(HINSTANCE hInstance) {
    g_App = new AppInstance();
    if (g_App->Initialize(hInstance)) {
        g_App->Run();
    }
    delete g_App;
    return 0;
}

} // namespace Application