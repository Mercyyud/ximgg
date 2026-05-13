#pragma once
#include <Windows.h>
#include <vector>
#include <mutex>
#include <imgui.h>

struct DetectionResult {
    ImVec2 min;
    ImVec2 max;
    float confidence;
    int classId;
};

class AIModule {
public:
    static bool Initialize();
    static void Shutdown();

    static void Start();
    static void Stop();
    static bool IsRunning();

    static std::vector<DetectionResult> GetLatestDetections();

    // Settings
    static float& GetConfidenceThreshold();
    static bool& GetEnabled();
    static ImVec4& GetBoxColor();

private:
    static void WorkerThread();
    static void CaptureScreen();

    static bool m_Running;
    static bool m_Enabled;
    static float m_ConfidenceThreshold;
    static ImVec4 m_BoxColor;

    static std::vector<DetectionResult> m_Detections;
    static std::mutex m_DetectionsMutex;
    static HANDLE m_ThreadHandle;
};
