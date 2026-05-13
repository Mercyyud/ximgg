/*
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 */

// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS

#include <windows.h>
#include <d3d11.h>
#include <chrono>
#include <tchar.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "gui.h"
#include "Fonts/Inter-SemiBold.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ID3D11Device*            g_pd3dDevice           = nullptr;
ID3D11DeviceContext*     g_pd3dDeviceContext     = nullptr;
IDXGISwapChain*          g_pSwapChain            = nullptr;
ID3D11RenderTargetView*  g_mainRenderTargetView  = nullptr;
HWND                     hwnd                    = nullptr;

bool   CreateDeviceD3D(HWND hWnd);
void   CleanupDeviceD3D();
void   CreateRenderTarget();
void   CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    WNDCLASSEX wc{};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = _T("ImGuiBase");
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassEx(&wc);

    hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        _T("ImGui Base"),
        WS_POPUP | WS_VISIBLE,
        100, 100,
        (int)window::size.x,
        (int)window::size.y,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    // Win11 smooth corners
    DWORD corner_preference = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &corner_preference, sizeof(corner_preference));

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImFontConfig font_config;
    font_config.FontDataOwnedByAtlas = false;
    font_config.OversampleH = 3;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = false;

    ImFont* font_body = io.Fonts->AddFontFromMemoryTTF(rawData, sizeof(rawData), 16.0f, &font_config);
    io.FontDefault = font_body;

    ImFontConfig title_config = font_config;
    ui::font_title = io.Fonts->AddFontFromMemoryTTF(rawData, sizeof(rawData), 22.0f, &title_config);

    ui::initialize();

    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedLines        = true;
    style.AntiAliasedFill         = true;
    style.WindowRounding          = window::rounding;
    style.FrameRounding           = 5.0f;
    style.TabRounding             = 5.0f;
    style.ScrollbarRounding       = 5.0f;
    style.WindowBorderSize        = 0.0f;
    style.FrameBorderSize         = 0.0f;
    style.AntiAliasedLinesUseTex  = false;
    style.Alpha                   = 1.0f;

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ui::render();

        ImGui::Render();

        const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); // VSync on
    }

    ui::shutdown();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount          = 2;
    sd.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow         = hWnd;
    sd.SampleDesc.Count     = 1;
    sd.Windowed             = TRUE;
    sd.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags                = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL featureLevel;
    if (D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            0, nullptr, 0, D3D11_SDK_VERSION,
            &sd, &g_pSwapChain, &g_pd3dDevice,
            &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    g_pd3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRenderTargetView);
    backBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release();        g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_NCHITTEST:
    {
        return HTCLIENT;
    }
    case WM_LBUTTONDOWN:
    {
        bool item_hovered = (ImGui::GetCurrentContext() != nullptr) && ImGui::IsAnyItemHovered();
        bool want_capture = (ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().WantCaptureMouse;
        if (item_hovered || want_capture)
        {
            break;
        }
        SetCapture(hWnd);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        POINT p;
        GetCursorPos(&p);
        static POINT last_p = { p.x, p.y };
        if (GetCapture() == hWnd && (wParam & MK_LBUTTON))
        {
            RECT r;
            GetWindowRect(hWnd, &r);
            SetWindowPos(hWnd, NULL, r.left + (p.x - last_p.x), r.top + (p.y - last_p.y), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        last_p = p;
        return 0;
    }
    case WM_LBUTTONUP:
    {
        if (GetCapture() == hWnd)
        {
            ReleaseCapture();
            return 0;
        }
        break;
    }
    case WM_ENTERSIZEMOVE:
        SetTimer(hWnd, 1, USER_TIMER_MINIMUM, NULL);
        return 0;
    case WM_EXITSIZEMOVE:
        KillTimer(hWnd, 1);
        return 0;
    case WM_TIMER:
        if (wParam == 1) SendMessage(hWnd, WM_PAINT, 0, 0);
        return 0;
    case WM_PAINT:
        if (g_pSwapChain && g_mainRenderTargetView && ImGui::GetCurrentContext() != nullptr)
        {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            ui::render();
            ImGui::Render();
            const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_pSwapChain->Present(1, 0);
            ValidateRect(hWnd, NULL);
        }
        else
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        return 0;
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
