#include "AutoDetect.h"
#include "../Operators/OperatorData.h"
#include "../Guns/GunData.h"
#include "../Config/Config.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include <map>
#include <algorithm>
#include <cstring>

namespace AutoDetect {

// Static state
static bool s_Initialized = false;
static bool s_Running = false;
static bool s_Enabled = false;
static bool s_AutoSwitchEnabled = true;
static int s_DetectionInterval = 2000; // 2 seconds
static DetectionCallback s_Callback = nullptr;

// Detection region (full screen by default)
static int s_DetectX = 0;
static int s_DetectY = 0;
static int s_DetectWidth = 0;
static int s_DetectHeight = 0;

// Latest result
static DetectionResult s_LatestResult;
static std::mutex s_ResultMutex;

// Thread handle
static std::thread* s_DetectionThread = nullptr;

// Helper: Capture screen region to buffer using standard GDI
static bool CaptureScreenRegion(std::vector<uint8_t>& buffer, int& width, int& height) {
    int x = s_DetectX;
    int y = s_DetectY;
    int w = s_DetectWidth;
    int h = s_DetectHeight;
    
    // If region not set, capture full screen
    if (w == 0 || h == 0) {
        x = 0;
        y = 0;
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);
    }
    
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, w, h);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    
    BitBlt(hdcMem, 0, 0, w, h, hdcScreen, x, y, SRCCOPY);
    
    // Get bitmap data
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // Negative for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    width = w;
    height = h;
    buffer.resize(w * h * 3);
    
    GetDIBits(hdcMem, hBitmap, 0, h, buffer.data(), &bmi, DIB_RGB_COLORS);
    
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    
    return true;
}

// Helper: Simple OCR using template matching for operator names
// This is a simplified approach - for production use Tesseract or similar would be better
static std::string DetectOperatorFromScreen(const std::vector<uint8_t>& buffer, int width, int height, float& confidence) {
    // These are approximate positions for the operator selection screen
    struct OperatorRegion {
        int x, y, w, h;
        const char* name;
    };
    
    // Common operator name regions (relative to screen size)
    float screenW = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
    float screenH = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
    
    // For now, we'll use a color-based detection approach
    // R6S operator cards have distinctive colors and patterns
    
    // Sample pixels from likely operator name area
    int sampleX = (int)(screenW * 0.5f); // Center of screen
    int sampleY = (int)(screenH * 0.3f); // Upper third
    
    if (sampleX >= width || sampleY >= height) {
        return "";
    }
    
    // Extract color signature from this region
    // This is a placeholder - real implementation would use proper OCR
    // For now, we'll return empty to indicate no detection
    
    return "";
}

// Helper: Detect gun name from screen
static std::string DetectGunFromScreen(const std::vector<uint8_t>& buffer, int width, int height, float& confidence) {
    (void)buffer;
    (void)width;
    (void)height;
    confidence = 0.0f;
    
    // Similar approach to operator detection
    // Gun names appear in specific UI regions
    
    return "";
}

// Alternative: Use memory reading for more reliable detection
// This reads game memory to get current operator and gun
static bool DetectFromMemory(std::string& operatorName, std::string& gunName) {
    (void)operatorName;
    (void)gunName;
    // This is a placeholder for memory-based detection
    // In a real implementation, you would:
    // 1. Find the game process (RainbowSix.exe)
    // 2. Read memory at known offsets for operator/gun data
    // 3. Parse the data to get names
    
    // For now, return false to indicate memory detection not available
    return false;
}

// Main detection logic
static void PerformDetection() {
    DetectionResult result;
    result.state = DetectionState::DetectingOperator;
    result.confidence = 0.0f;
    result.timestamp = GetTickCount64();
    
    // Try memory-based detection first (more reliable)
    if (DetectFromMemory(result.detectedOperator, result.detectedGun)) {
        result.state = DetectionState::Success;
        result.confidence = 1.0f;
    } else {
        // Fall back to screen-based detection
        std::vector<uint8_t> buffer;
        int width, height;
        
        if (CaptureScreenRegion(buffer, width, height)) {
            float opConfidence = 0.0f;
            result.detectedOperator = DetectOperatorFromScreen(buffer, width, height, opConfidence);
            
            if (!result.detectedOperator.empty()) {
                result.state = DetectionState::DetectingGun;
                float gunConfidence = 0.0f;
                result.detectedGun = DetectGunFromScreen(buffer, width, height, gunConfidence);
                
                result.confidence = (opConfidence + gunConfidence) / 2.0f;
                result.state = DetectionState::Success;
            } else {
                result.state = DetectionState::Failed;
            }
        } else {
            result.state = DetectionState::Failed;
        }
    }
    
    // Update latest result
    {
        std::lock_guard<std::mutex> lock(s_ResultMutex);
        s_LatestResult = result;
    }
    
    // Call callback if set
    if (s_Callback) {
        s_Callback(result);
    }
    
    // Auto-switch if enabled and detection succeeded
    if (s_AutoSwitchEnabled && result.state == DetectionState::Success) {
        if (!result.detectedOperator.empty()) {
            const OperatorData::Operator* op = OperatorData::Find(result.detectedOperator);
            if (op) {
                // Trigger profile switch (this would need to be hooked up to Menu)
                // For now, we'll save to config
                Config::Settings settings = Config::LoadSettings();
                settings.lastOperator = result.detectedOperator;
                Config::SaveSettings(settings);
            }
        }
    }
}

// Detection thread
static void DetectionThreadFunc() {
    while (s_Running) {
        if (!s_Enabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        
        PerformDetection();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(s_DetectionInterval));
    }
}

// Public implementation
bool Initialize() {
    if (s_Initialized) return true;
    
    // Initialize latest result
    s_LatestResult.state = DetectionState::Idle;
    s_LatestResult.confidence = 0.0f;
    s_LatestResult.timestamp = 0;
    
    s_Initialized = true;
    return true;
}

void Shutdown() {
    if (!s_Initialized) return;
    
    Stop();
    
    s_Initialized = false;
}

void Start() {
    if (!s_Initialized || s_Running) return;
    
    s_Running = true;
    s_DetectionThread = new std::thread(DetectionThreadFunc);
}

void Stop() {
    s_Running = false;
    s_Enabled = false;
    
    if (s_DetectionThread) {
        s_DetectionThread->join();
        delete s_DetectionThread;
        s_DetectionThread = nullptr;
    }
}

bool IsRunning() {
    return s_Running && s_Enabled;
}

void TriggerDetection() {
    if (!s_Initialized) return;
    
    // Perform detection in a separate thread to avoid blocking
    std::thread([]() {
        PerformDetection();
    }).detach();
}

DetectionResult GetLatestResult() {
    std::lock_guard<std::mutex> lock(s_ResultMutex);
    return s_LatestResult;
}

void SetCallback(DetectionCallback callback) {
    s_Callback = callback;
}

void SetDetectionInterval(int milliseconds) {
    s_DetectionInterval = milliseconds;
}

void SetEnabled(bool enabled) {
    s_Enabled = enabled;
}

bool IsEnabled() {
    return s_Enabled;
}

void SetDetectionRegion(int x, int y, int width, int height) {
    s_DetectX = x;
    s_DetectY = y;
    s_DetectWidth = width;
    s_DetectHeight = height;
}

void GetDetectionRegion(int& x, int& y, int& width, int& height) {
    x = s_DetectX;
    y = s_DetectY;
    width = s_DetectWidth;
    height = s_DetectHeight;
}

void SetAutoSwitchEnabled(bool enabled) {
    s_AutoSwitchEnabled = enabled;
}

bool IsAutoSwitchEnabled() {
    return s_AutoSwitchEnabled;
}

} // namespace AutoDetect
