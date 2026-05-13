#include "Mouse.h"

#include <Windows.h>
#include <winternl.h>
#include <SetupAPI.h>
#include <hidsdi.h>
#include <cstring>
#include <string>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace Mouse {

static HANDLE g_Device    = INVALID_HANDLE_VALUE;
static bool   g_Connected = false;

struct MouseReport {
    char button;
    char x;
    char y;
    char wheel;
    char unk1;
};

static HANDLE OpenLogitechDevice() {
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    // First pass: look for col01 (primary mouse interface)
    for (int pass = 0; pass < 2; ++pass) {
        HDEVINFO devInfo = SetupDiGetClassDevsA(
            &hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (devInfo == INVALID_HANDLE_VALUE)
            return INVALID_HANDLE_VALUE;

        SP_DEVICE_INTERFACE_DATA iface = {};
        iface.cbSize = sizeof(iface);

        for (DWORD i = 0;
             SetupDiEnumDeviceInterfaces(devInfo, NULL, &hidGuid, i, &iface);
             ++i) {
            DWORD needed = 0;
            SetupDiGetDeviceInterfaceDetailA(devInfo, &iface, NULL, 0, &needed, NULL);
            if (!needed) continue;

            auto detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)malloc(needed);
            if (!detail) continue;
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

            if (SetupDiGetDeviceInterfaceDetailA(devInfo, &iface, detail, needed, NULL, NULL)) {
                std::string path(detail->DevicePath);
                for (auto& c : path) c = (char)tolower(c);

                bool isLogitech = path.find("vid_046d") != std::string::npos;
                bool isCol01    = path.find("col01")    != std::string::npos;

                if (isLogitech && (pass == 1 || isCol01)) {
                    HANDLE dev = CreateFileA(detail->DevicePath,
                        GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, 0, NULL);
                    if (dev != INVALID_HANDLE_VALUE) {
                        free(detail);
                        SetupDiDestroyDeviceInfoList(devInfo);
                        return dev;
                    }
                }
            }
            free(detail);
        }
        SetupDiDestroyDeviceInfoList(devInfo);
    }
    return INVALID_HANDLE_VALUE;
}

bool Open() {
    if (g_Connected) return true;
    g_Device = OpenLogitechDevice();
    if (g_Device != INVALID_HANDLE_VALUE) {
        g_Connected = true;
        return true;
    }
    return false; // will fall back to SendInput in Move()
}

void Move(int x, int y) {
    // Clamp to signed byte range
    if (x >  127) x =  127;
    if (x < -127) x = -127;
    if (y >  127) y =  127;
    if (y < -127) y = -127;

    bool sent = false;

    if (g_Connected && g_Device != INVALID_HANDLE_VALUE) {
        MouseReport report = {};
        report.x = (char)x;
        report.y = (char)y;

        DWORD written = 0;
        if (HidD_SetOutputReport(g_Device, &report, sizeof(report)))
            sent = true;
        else if (WriteFile(g_Device, &report, sizeof(report), &written, NULL))
            sent = true;
    }

    if (!sent) {
        INPUT input      = {};
        input.type       = INPUT_MOUSE;
        input.mi.dx      = x;
        input.mi.dy      = y;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &input, sizeof(INPUT));
    }
}

void Close() {
    if (g_Device != INVALID_HANDLE_VALUE) {
        CloseHandle(g_Device);
        g_Device = INVALID_HANDLE_VALUE;
    }
    g_Connected = false;
}

bool IsConnected() { return g_Connected; }

} // namespace Mouse
