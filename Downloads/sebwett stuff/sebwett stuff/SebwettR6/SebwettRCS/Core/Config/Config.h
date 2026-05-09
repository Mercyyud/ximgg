#pragma once
#include <string>
#include <Windows.h>

namespace Config {

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
    std::string lastOperator = "";
};

// Profile persistence
bool    SaveProfile(const Profile& profile);
Profile LoadProfile(const std::string& operatorName);

// Settings persistence
bool     SaveSettings(const Settings& settings);
Settings LoadSettings();

} // namespace Config
