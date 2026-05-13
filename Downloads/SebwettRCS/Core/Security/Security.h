#pragma once
#include <Windows.h>

namespace Security {
    // Run all protection checks
    bool RunChecks();

    // Individual layers
    bool CheckDebugger();
    bool CheckVM();
    bool CheckIntegrity();
    
    // Automatic protection thread
    void Initialize();
}
