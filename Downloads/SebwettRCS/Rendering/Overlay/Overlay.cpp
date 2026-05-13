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
    return true;
}

void Resize(int width, int height) {
    if (!g_SwapChain) return;
    DestroyRenderTarget();
    g_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

void Shutdown() {
    DestroyRenderTarget();
    if (g_SwapChain) { g_SwapChain->Release(); g_SwapChain = NULL; }
    if (g_Context)   { g_Context->Release();   g_Context   = NULL; }
    if (g_Device)    { g_Device->Release();     g_Device    = NULL; }
}

void BeginFrame() {
}

void EndFrame() {
    const float clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // fully transparent
    g_Context->OMSetRenderTargets(1, &g_RenderTargetView, NULL);
    g_Context->ClearRenderTargetView(g_RenderTargetView, clear);
}

void Present() {
    g_SwapChain->Present(1, 0);
}

void SetVisible(bool visible, bool takeFocus) {
    g_Visible = visible;
    if (!g_Hwnd) return;

    // Use SetWindowLongPtrW for 64-bit compatibility
    LONG_PTR ex = GetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE);
    LONG_PTR oldEx = ex;

    if (visible) {
        // Remove click-through so user can interact with the menu
        ex &= ~WS_EX_TRANSPARENT;
        
        if (takeFocus) {
            ex &= ~WS_EX_NOACTIVATE;
        } else {
            // Keep NOACTIVATE so we don't steal focus from R6 when clicking the UI
            ex |= WS_EX_NOACTIVATE;
        }
    } else {
        // Restore click-through and no-activate when menu is hidden
        ex |= WS_EX_TRANSPARENT;
        ex |= WS_EX_NOACTIVATE;
    }

    if (ex != oldEx) {
        SetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE, ex);
        
        // SWP_FRAMECHANGED is critical after changing GWL_EXSTYLE to ensure Windows updates the window state
        SetWindowPos(g_Hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
}

bool          IsVisible() { return g_Visible; }

void SetStreamProof(bool enabled) {
    if (!g_Hwnd) return;
    SetWindowDisplayAffinity(g_Hwnd, enabled ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
}
HWND          GetHWND()   { return g_Hwnd;    }
ID3D11Device* GetDevice() { return g_Device;  }
ID3D11DeviceContext* GetContext() { return g_Context; }

} // namespace Overlay
