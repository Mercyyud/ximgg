#include "AIModule.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <onnxruntime_cxx_api.h>

bool AIModule::m_Running = false;
bool AIModule::m_Enabled = false;
float AIModule::m_ConfidenceThreshold = 0.5f;
ImVec4 AIModule::m_BoxColor = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
std::vector<DetectionResult> AIModule::m_Detections;
std::mutex AIModule::m_DetectionsMutex;
HANDLE AIModule::m_ThreadHandle = NULL;

// ONNX Runtime session
static Ort::Env* ort_env = nullptr;
static Ort::Session* ort_session = nullptr;
static Ort::MemoryInfo* ort_memory_info = nullptr;

bool AIModule::Initialize() {
    // AI ESP disabled
    return true;
}

void AIModule::Shutdown() {
    m_Running = false;
    if (m_ThreadHandle) {
        WaitForSingleObject(m_ThreadHandle, 1000);
        CloseHandle(m_ThreadHandle);
        m_ThreadHandle = NULL;
    }
    
    delete ort_session;
    delete ort_env;
    delete ort_memory_info;
}

void AIModule::Start() { m_Enabled = true; }
void AIModule::Stop() { m_Enabled = false; }
bool AIModule::IsRunning() { return m_Enabled; }

std::vector<DetectionResult> AIModule::GetLatestDetections() {
    std::lock_guard<std::mutex> lock(m_DetectionsMutex);
    return m_Detections;
}

float& AIModule::GetConfidenceThreshold() { return m_ConfidenceThreshold; }
bool& AIModule::GetEnabled() { return m_Enabled; }
ImVec4& AIModule::GetBoxColor() { return m_BoxColor; }

void AIModule::CaptureScreen() {
    // Screen capture is done in WorkerThread
}

void AIModule::WorkerThread() {
    // AI ESP disabled - no detection
}
