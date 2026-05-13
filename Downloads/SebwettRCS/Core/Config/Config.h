#pragma once
#include <string>
#include <Windows.h>

namespace Config {

// Auto-detect region settings
struct DetectRegion {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

// Registry root for all saved data
constexpr const wchar_t* REGISTRY_ROOT = L"Software\\XimGG";

// Per-operator recoil profile
struct Profile {
    std::string operatorName;
    float recoilX = 0.0f;
    float recoilY = 20.0f;
};

// Global application settings
struct Settings {
    int  menuHotkey    = VK_INSERT; // key that opens/closes the menu
    bool recoilEnabled = true;
    bool onlyADS       = true;      // only compensate while right-click is held
    bool onlyR6        = true;      // only compensate while R6 is foreground
    bool streamProof   = true;      // hide from capture (Discord/OBS)
    bool rememberKey   = false;     // save license key
    int  recoilHotkey  = 0;         // hotkey to toggle recoil (0 = none)
    std::string licenseKey = "";    // saved license key
    std::string lastOperator = "";
    std::string lastGun      = "";  // last selected gun for the active operator
    // Auto-detect settings
    bool autoDetectEnabled = false;     // enable auto-detection of operator/gun
    bool autoSwitchEnabled = true;      // auto-switch profile when detected
    int  detectionInterval = 2000;      // detection interval in milliseconds
};

// Profile persistence
bool    SaveProfile(const Profile& profile);
Profile LoadProfile(const std::string& operatorName);

// Settings persistence
bool     SaveSettings(const Settings& settings);
Settings LoadSettings();

} // namespace Config
