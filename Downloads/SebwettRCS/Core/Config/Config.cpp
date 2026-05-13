#include "Config.h"
#include <Windows.h>
#include <string>

namespace Config {

// ── Registry helpers ──────────────────────────────────────────────────────

static std::wstring ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

static std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, NULL, NULL);
    return s;
}

static void WriteFloat(HKEY k, const wchar_t* name, float v) {
    DWORD d = *(DWORD*)&v;
    RegSetValueExW(k, name, 0, REG_DWORD, (BYTE*)&d, sizeof(DWORD));
}

static float ReadFloat(HKEY k, const wchar_t* name, float def) {
    DWORD d = 0, sz = sizeof(DWORD), type = 0;
    if (RegQueryValueExW(k, name, NULL, &type, (BYTE*)&d, &sz) == ERROR_SUCCESS && type == REG_DWORD)
        return *(float*)&d;
    return def;
}

static void WriteInt(HKEY k, const wchar_t* name, int v) {
    DWORD d = (DWORD)v;
    RegSetValueExW(k, name, 0, REG_DWORD, (BYTE*)&d, sizeof(DWORD));
}

static int ReadInt(HKEY k, const wchar_t* name, int def) {
    DWORD d = 0, sz = sizeof(DWORD), type = 0;
    if (RegQueryValueExW(k, name, NULL, &type, (BYTE*)&d, &sz) == ERROR_SUCCESS && type == REG_DWORD)
        return (int)d;
    return def;
}

static void WriteString(HKEY k, const wchar_t* name, const std::string& v) {
    std::wstring w = ToWide(v);
    RegSetValueExW(k, name, 0, REG_SZ, (BYTE*)w.c_str(),
                   (DWORD)((w.size() + 1) * sizeof(wchar_t)));
}

static std::string ReadString(HKEY k, const wchar_t* name, const std::string& def) {
    DWORD sz = 0, type = 0;
    if (RegQueryValueExW(k, name, NULL, &type, NULL, &sz) == ERROR_SUCCESS && type == REG_SZ) {
        std::wstring w(sz / sizeof(wchar_t), 0);
        if (RegQueryValueExW(k, name, NULL, NULL, (BYTE*)&w[0], &sz) == ERROR_SUCCESS) {
            while (!w.empty() && w.back() == L'\0') w.pop_back();
            return ToUtf8(w);
        }
    }
    return def;
}

// ── Profile ───────────────────────────────────────────────────────────────

bool SaveProfile(const Profile& p) {
    std::wstring subkey = std::wstring(REGISTRY_ROOT) + L"\\Profiles\\" + ToWide(p.operatorName);
    HKEY key; DWORD disp;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subkey.c_str(), 0, NULL, 0,
                        KEY_WRITE, NULL, &key, &disp) != ERROR_SUCCESS)
        return false;

    WriteFloat(key, L"RecoilX", p.recoilX);
    WriteFloat(key, L"RecoilY", p.recoilY);
    RegCloseKey(key);
    return true;
}

Profile LoadProfile(const std::string& operatorName) {
    Profile p;
    p.operatorName = operatorName;

    std::wstring subkey = std::wstring(REGISTRY_ROOT) + L"\\Profiles\\" + ToWide(operatorName);
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subkey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
        return p; // return defaults

    p.recoilX = ReadFloat(key, L"RecoilX", 0.0f);
    p.recoilY = ReadFloat(key, L"RecoilY", 20.0f);
    RegCloseKey(key);
    return p;
}

// ── Settings ──────────────────────────────────────────────────────────────

bool SaveSettings(const Settings& s) {
    HKEY key; DWORD disp;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_ROOT, 0, NULL, 0,
                        KEY_WRITE, NULL, &key, &disp) != ERROR_SUCCESS)
        return false;

    WriteInt   (key, L"MenuHotkey",    s.menuHotkey);
    WriteInt   (key, L"RecoilEnabled", s.recoilEnabled ? 1 : 0);
    WriteInt   (key, L"OnlyADS",       s.onlyADS       ? 1 : 0);
    WriteInt   (key, L"OnlyR6",        s.onlyR6        ? 1 : 0);
    WriteInt   (key, L"StreamProof",   s.streamProof   ? 1 : 0);
    WriteInt   (key, L"RememberKey",   s.rememberKey   ? 1 : 0);
    WriteInt   (key, L"RecoilHotkey",  s.recoilHotkey);
    WriteString(key, L"LicenseKey",    s.licenseKey);
    WriteString(key, L"LastOperator",  s.lastOperator);
    WriteString(key, L"LastGun",       s.lastGun);
    WriteInt   (key, L"AutoDetectEnabled", s.autoDetectEnabled ? 1 : 0);
    WriteInt   (key, L"AutoSwitchEnabled", s.autoSwitchEnabled ? 1 : 0);
    WriteInt   (key, L"DetectionInterval", s.detectionInterval);
    RegCloseKey(key);
    return true;
}

Settings LoadSettings() {
    Settings s;
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_ROOT, 0, KEY_READ, &key) != ERROR_SUCCESS)
        return s;

    s.menuHotkey    = ReadInt   (key, L"MenuHotkey",    VK_INSERT);
    s.recoilEnabled = ReadInt   (key, L"RecoilEnabled", 1) != 0;
    s.onlyADS       = ReadInt   (key, L"OnlyADS",       1) != 0;
    s.onlyR6        = ReadInt   (key, L"OnlyR6",        1) != 0;
    s.streamProof   = ReadInt   (key, L"StreamProof",   1) != 0;
    s.rememberKey   = ReadInt   (key, L"RememberKey",   0) != 0;
    s.recoilHotkey  = ReadInt   (key, L"RecoilHotkey",  0);
    s.autoDetectEnabled = ReadInt(key, L"AutoDetectEnabled", 0) != 0;
    s.autoSwitchEnabled = ReadInt(key, L"AutoSwitchEnabled", 1) != 0;
    s.detectionInterval = ReadInt(key, L"DetectionInterval", 2000);
    s.licenseKey    = ReadString(key, L"LicenseKey",    "");
    s.lastOperator  = ReadString(key, L"LastOperator",  "");
    s.lastGun       = ReadString(key, L"LastGun",       "");
    RegCloseKey(key);
    return s;
}

} // namespace Config
