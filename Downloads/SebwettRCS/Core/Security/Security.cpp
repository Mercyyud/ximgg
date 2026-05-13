#include "Security.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

namespace Security {

    bool CheckDebugger() {
        // Standard Win32 Check
        if (IsDebuggerPresent()) return true;

        // Check for remote debugger
        BOOL remoteDebugger = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDebugger);
        if (remoteDebugger) return true;

        // Timing check (RDTSC) - debuggers slow down execution
        unsigned long long t1 = __rdtsc();
        for (int i = 0; i < 100; i++) GetTickCount();
        unsigned long long t2 = __rdtsc();
        if ((t2 - t1) > 0x100000) return true;

        return false;
    }

    bool CheckVM() {
        // Simple check for common VM files/drivers
        const char* vmFiles[] = {
            "C:\\windows\\System32\\Drivers\\VBoxMouse.sys",
            "C:\\windows\\System32\\Drivers\\VBoxGuest.sys",
            "C:\\windows\\System32\\Drivers\\vmtoolsd.sys",
            "C:\\windows\\System32\\Drivers\\vmmouse.sys"
        };

        for (auto file : vmFiles) {
            if (GetFileAttributesA(file) != INVALID_FILE_ATTRIBUTES) return true;
        }

        return false;
    }

    bool CheckIntegrity() {
        // This would ideally hash the .text section
        // For now, we'll keep it as a placeholder for expansion
        return false;
    }

    bool RunChecks() {
        if (CheckDebugger()) return true;
        if (CheckVM()) return true;
        return false;
    }

    void ProtectionThread() {
        while (true) {
            if (RunChecks()) {
                // Nuclear option: Kill process immediately
                TerminateProcess(GetCurrentProcess(), 0xDEAD);
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void Initialize() {
        std::thread(ProtectionThread).detach();
    }
}
