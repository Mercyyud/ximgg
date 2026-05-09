#pragma once
#include <Windows.h>
#include <d3d11.h>

namespace Overlay {

// Create the DX11 device, swap chain, and initialise ImGui backends.
// hwnd must be a valid layered/transparent window.
bool Initialize(HWND hwnd);

// Tear down ImGui and DX11 resources.
void Shutdown();

// Call before any ImGui rendering each frame.
void BeginFrame();

// Call after all ImGui rendering to present the frame.
void EndFrame();

// Show/hide the menu.  When hidden the window is click-through (WS_EX_TRANSPARENT).
void SetVisible(bool visible);
bool IsVisible();

// Accessors
HWND              GetHWND();
ID3D11Device*     GetDevice();

} // namespace Overlay
