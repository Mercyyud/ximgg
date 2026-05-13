#pragma once
#include <string>
#include <functional>
#include <mutex>
#include <thread>

namespace AutoDetect {

enum class DetectionState {
    Idle,
    DetectingOperator,
    DetectingGun,
    Success,
    Failed
};

struct DetectionResult {
    DetectionState state;
    std::string detectedOperator;
    std::string detectedGun;
    float confidence;
    uint64_t timestamp;
};

// Callback for when detection completes
using DetectionCallback = std::function<void(const DetectionResult&)>;

// Initialize the auto-detect system
bool Initialize();

// Shutdown the auto-detect system
void Shutdown();

// Start auto-detection (runs in background)
void Start();

// Stop auto-detection
void Stop();

// Check if auto-detection is running
bool IsRunning();

// Manually trigger a detection scan
void TriggerDetection();

// Get the latest detection result
DetectionResult GetLatestResult();

// Set callback for detection events
void SetCallback(DetectionCallback callback);

// Settings
void SetDetectionInterval(int milliseconds); // Default: 2000ms
void SetEnabled(bool enabled);
bool IsEnabled();

// Screen capture region (set to detect specific UI area)
void SetDetectionRegion(int x, int y, int width, int height);
void GetDetectionRegion(int& x, int& y, int& width, int& height);

// Auto-switch settings
void SetAutoSwitchEnabled(bool enabled);
bool IsAutoSwitchEnabled();

} // namespace AutoDetect
