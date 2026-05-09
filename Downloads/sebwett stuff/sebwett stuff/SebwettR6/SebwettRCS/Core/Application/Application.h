#pragma once
#include <Windows.h>

namespace Application {

// Create the transparent overlay window, initialise all subsystems,
// and run the message + render loop until the user closes the app.
int Run(HINSTANCE hInstance);

// Window procedure (must be accessible to WinMain)
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

} // namespace Application
