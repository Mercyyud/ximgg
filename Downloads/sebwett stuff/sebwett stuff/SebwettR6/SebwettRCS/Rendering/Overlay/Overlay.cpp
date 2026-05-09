#include "Overlay.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

namespace Overlay {

static HWND                     g_Hwnd                = NULL;
static ID3D11Device*            g_Device              = NULL;
static ID3D11DeviceContext*     g_Context             = NULL;
static IDXGISwapChain*          g_SwapChain           = NULL;
static ID3D11RenderTargetView*  g_RenderTargetView    = NULL;
static bool                     g_Visible             = false;

static void CreateRenderTarget() {
    ID3D11Texture2D* back = nullptr;
    g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        g_Device->CreateRenderTargetView(back, NULL, &g_RenderTargetView);
        back->Release();
    }
}

static void DestroyRenderTarget() {
    if (g_RenderTargetView) { g_RenderTargetView->Release(); g_RenderTargetView = NULL; }
}

bool Initialize(HWND hwnd) {
    g_Hwnd = hwnd;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        levels, 2, D3D11_SDK_VERSION,
        &sd, &g_SwapChain, &g_Device, &featureLevel, &g_Context);
    if (FAILED(hr)) return false;

    CreateRenderTarget();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags  |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename   = NULL; // no imgui.ini

    // Load Segoe UI if available, otherwise ImGui default
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 17.0f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_Device, g_Context);

    return true;
}

void Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    DestroyRenderTarget();
    if (g_SwapChain) { g_SwapChain->Release(); g_SwapChain = NULL; }
    if (g_Context)   { g_Context->Release();   g_Context   = NULL; }
    if (g_Device)    { g_Device->Release();     g_Device    = NULL; }
}

void BeginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void EndFrame() {
    ImGui::Render();

    const float clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // fully transparent
    g_Context->OMSetRenderTargets(1, &g_RenderTargetView, NULL);
    g_Context->ClearRenderTargetView(g_RenderTargetView, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_SwapChain->Present(1, 0);
}

void SetVisible(bool visible) {
    g_Visible = visible;
    if (!g_Hwnd) return;

    LONG ex = GetWindowLong(g_Hwnd, GWL_EXSTYLE);
    ex |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    SetWindowLong(g_Hwnd, GWL_EXSTYLE, ex);

    if (visible) {
        SetWindowPos(
            g_Hwnd,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    } else {
        SetWindowPos(
            g_Hwnd,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

bool          IsVisible() { return g_Visible; }
HWND          GetHWND()   { return g_Hwnd;    }
ID3D11Device* GetDevice() { return g_Device;  }

} // namespace Overlay
