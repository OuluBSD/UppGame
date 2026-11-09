#include "DebuggingTools.h"

NAMESPACE_UPP

FpsMemoryTracker::FpsMemoryTracker() {
    // Capture initial memory usage
    initialMemory = GetUsedMemory();
}

FpsMemoryTracker::~FpsMemoryTracker() {
}

bool FpsMemoryTracker::Initialize() {
    frameTimes.Reserve(MAX_FRAME_TIMES);
    return true;
}

void FpsMemoryTracker::Update() {
    uint64 currentTime = GetTickCount();
    
    // Update frame time
    if (lastFrameTime > 0) {
        uint64 frameTime = currentTime - lastFrameTime;
        
        // Add to frame time history
        frameTimes.Add(frameTime);
        if (frameTimes.GetCount() > MAX_FRAME_TIMES) {
            frameTimes.Remove(0);
        }
        
        // Calculate FPS
        if (frameTime > 0) {
            currentFPS = 1000.0 / frameTime;  // Convert to FPS (milliseconds to seconds)
        }
        
        // Update average FPS
        if (!frameTimes.IsEmpty()) {
            uint64 totalTime = 0;
            for (auto ft : frameTimes) {
                totalTime += ft;
            }
            if (totalTime > 0) {
                averageFPS = (frameTimes.GetCount() * 1000.0) / totalTime;
            }
        }
        
        // Update frame time statistics
        double frameTimeSec = frameTime / 1000.0;
        frameTimeMin = min(frameTimeMin, frameTimeSec);
        frameTimeMax = max(frameTimeMax, frameTimeSec);
        
        frameTimeHistory.Add(frameTimeSec);
        if (frameTimeHistory.GetCount() > MAX_FRAME_TIMES) {
            frameTimeHistory.Remove(0);
        }
        
        double sum = 0;
        for (double ft : frameTimeHistory) {
            sum += ft;
        }
        frameTimeAvg = sum / frameTimeHistory.GetCount();
    }
    
    lastFrameTime = currentTime;
    frameCount++;
    
    // Update memory tracking
    int64 currentMemory = GetUsedMemory();
    peakMemory = max(peakMemory, currentMemory);
}

int64 FpsMemoryTracker::GetUsedMemory() const {
    // In a real implementation, this would return actual memory usage
    // For this implementation, we'll return a simulated value
    return 1024 * 1024 * (50 + (frameCount % 100)); // Simulated memory usage in bytes
}

int64 FpsMemoryTracker::GetTotalMemory() const {
    // Return a simulated total memory (e.g., 8GB)
    return 1024L * 1024L * 1024L * 8L; // 8GB in bytes
}

double FpsMemoryTracker::GetMemoryUsagePercent() const {
    int64 total = GetTotalMemory();
    if (total > 0) {
        return (double)GetUsedMemory() / total * 100.0;
    }
    return 0.0;
}

void FpsMemoryTracker::Reset() {
    frameTimes.Clear();
    currentFPS = 0.0;
    averageFPS = 0.0;
    frameCount = 0;
    lastFrameTime = 0;
    
    frameTimeMin = 1000.0;
    frameTimeMax = 0.0;
    frameTimeAvg = 0.0;
    frameTimeHistory.Clear();
}

FrameAnalyzer::FrameAnalyzer() {
}

FrameAnalyzer::~FrameAnalyzer() {
}

bool FrameAnalyzer::Initialize() {
    return true;
}

void FrameAnalyzer::BeginFrame() {
    frameStartTime = GetTickCount();
    inFrame = true;
    Reset();
}

void FrameAnalyzer::EndFrame() {
    if (inFrame) {
        uint64 endTime = GetTickCount();
        double totalTime = (endTime - frameStartTime) / 1000.0;  // Convert to seconds
        
        // Calculate time spent in different activities
        otherTime = totalTime - renderTime - updateTime;
        if (otherTime < 0) otherTime = 0;  // Guard against negative values
        
        inFrame = false;
    }
}

void FrameAnalyzer::RegisterDrawCall(int vertexCount, int triangleCount) {
    drawCallCount++;
    totalVertexCount += vertexCount;
    totalTriangleCount += triangleCount;
}

void FrameAnalyzer::RegisterTextureBind(int textureId) {
    textureBindCount++;
}

void FrameAnalyzer::RegisterShaderBind(int shaderId) {
    shaderBindCount++;
}

void FrameAnalyzer::Reset() {
    drawCallCount = 0;
    totalVertexCount = 0;
    totalTriangleCount = 0;
    textureBindCount = 0;
    shaderBindCount = 0;
    
    renderTime = 0.0;
    updateTime = 0.0;
    otherTime = 0.0;
}

AllocationProfiler::AllocationProfiler() {
}

AllocationProfiler::~AllocationProfiler() {
    Reset();
}

bool AllocationProfiler::Initialize() {
    return true;
}

void AllocationProfiler::TrackAllocation(void* ptr, size_t size, const String& source) {
    if (!enabled) return;
    
    Mutex::Lock lock(cs);
    
    AllocationInfo info;
    info.ptr = ptr;
    info.size = size;
    info.source = source;
    info.timestamp = GetTickCount();
    
    allocations.GetAdd(ptr) = info;
    
    totalAllocated += size;
    peakAllocated = max(peakAllocated, totalAllocated);
    allocationCount++;
}

void AllocationProfiler::TrackDeallocation(void* ptr) {
    if (!enabled) return;
    
    Mutex::Lock lock(cs);
    
    auto it = allocations.GetIter(ptr);
    if (it) {
        totalAllocated -= it().size;
        allocations.Remove(ptr);
    }
}

Vector<AllocationProfiler::AllocationInfo> AllocationProfiler::GetAllocations() const {
    Vector<AllocationInfo> result;
    
    Mutex::Lock lock(const_cast<CriticalSection&>(cs));
    
    for (const auto& pair : allocations) {
        result.Add(pair.value);
    }
    
    return result;
}

void AllocationProfiler::Reset() {
    Mutex::Lock lock(cs);
    
    allocations.Clear();
    totalAllocated = 0;
    peakAllocated = 0;
    allocationCount = 0;
}

PerformanceWarningSystem::PerformanceWarningSystem() {
}

PerformanceWarningSystem::~PerformanceWarningSystem() {
}

bool PerformanceWarningSystem::Initialize() {
    return true;
}

void PerformanceWarningSystem::CheckPerformance() {
    if (!enabled) return;
    
    warnings.Clear();
    
    // This would check actual performance metrics if we had access to them
    // For this implementation, we'll just check the monitored metrics
    
    for (const auto& metric : monitoredMetrics) {
        if (metric.value > metric.threshold) {
            warnings.Add(metric.warningMessage);
        }
    }
    
    // Add some simulated warnings based on common thresholds
    if (rand() % 1000 < 2) { // Randomly add a warning occasionally
        warnings.Add("Simulated performance warning: High draw call count detected");
    }
}

void PerformanceWarningSystem::MonitorMetric(const String& name, double value, double threshold, 
                                            const String& warningMessage) {
    MonitoredMetric metric;
    metric.name = name;
    metric.value = value;
    metric.threshold = threshold;
    metric.warningMessage = warningMessage;
    
    monitoredMetrics.Add(metric);
}

void PerformanceWarningSystem::ClearWarnings() {
    warnings.Clear();
}

DebuggingToolsUI::DebuggingToolsUI() {
}

DebuggingToolsUI::~DebuggingToolsUI() {
}

bool DebuggingToolsUI::Initialize() {
    return true;
}

void DebuggingToolsUI::Update(double dt) {
    if (!visible) return;
    
    // Update layout positions
    int x = 10;
    int y = 10;
    int lineHeight = 20;
    
    if (showFpsCounter && fpsTracker) {
        fpsRect = Rect(x, y, x + 200, y + lineHeight);
        y += lineHeight + 5;
    }
    
    if (showMemoryUsage && fpsTracker) {
        memoryRect = Rect(x, y, x + 300, y + lineHeight);
        y += lineHeight + 5;
    }
    
    if (showFrameStats && frameAnalyzer) {
        statsRect = Rect(x, y, x + 300, y + lineHeight * 6); // 6 lines of stats
        y += lineHeight * 6 + 5;
    }
    
    if (showPerformanceWarnings && performanceWarningSystem) {
        performanceWarningSystem->CheckPerformance();
        Vector<String> warnings = performanceWarningSystem->GetWarnings();
        warningsRect = Rect(x, y, x + 400, y + lineHeight * (warnings.GetCount() + 1));
    }
}

void DebuggingToolsUI::Render(Draw& draw, const Rect& viewport) {
    if (!visible) return;
    
    // Render FPS counter
    if (showFpsCounter && fpsTracker) {
        String fpsText = String("FPS: ") + 
                        IntStr((int)fpsTracker->GetFPS()) + 
                        String(" (Avg: ") + 
                        IntStr((int)fpsTracker->GetAverageFPS()) + 
                        String(")");
        draw.DrawText(fpsRect.left, fpsRect.top, fpsText, StdFont(), White());
    }
    
    // Render memory usage
    if (showMemoryUsage && fpsTracker) {
        int64 used = fpsTracker->GetUsedMemory();
        int64 total = fpsTracker->GetTotalMemory();
        String memText = String("Memory: ") + 
                        IntStr((int)(used / (1024 * 1024))) + 
                        String("MB (") + 
                        String().Cat() << fpsTracker->GetMemoryUsagePercent() << 
                        String("%. Peak: ") + 
                        IntStr((int)(fpsTracker->GetPeakAllocated() / (1024 * 1024))) + 
                        String("MB)");
        draw.DrawText(memoryRect.left, memoryRect.top, memText, StdFont(), White());
    }
    
    // Render frame statistics
    if (showFrameStats && frameAnalyzer) {
        int y = statsRect.top;
        draw.DrawText(statsRect.left, y, String("Frame Stats:"), StdFont(), White());
        y += lineHeight;
        
        draw.DrawText(statsRect.left, y, String("Draw Calls: ") + IntStr(frameAnalyzer->GetDrawCallCount()), StdFont(), White());
        y += lineHeight;
        
        draw.DrawText(statsRect.left, y, String("Vertices: ") + IntStr(frameAnalyzer->GetTotalVertexCount()), StdFont(), White());
        y += lineHeight;
        
        draw.DrawText(statsRect.left, y, String("Triangles: ") + IntStr(frameAnalyzer->GetTotalTriangleCount()), StdFont(), White());
        y += lineHeight;
        
        draw.DrawText(statsRect.left, y, String("Render Time: ") + String().Cat() << frameAnalyzer->GetRenderTime() << String("ms"), StdFont(), White());
        y += lineHeight;
        
        draw.DrawText(statsRect.left, y, String("Update Time: ") + String().Cat() << frameAnalyzer->GetUpdateTime() << String("ms"), StdFont(), White());
    }
    
    // Render performance warnings
    if (showPerformanceWarnings && performanceWarningSystem) {
        Vector<String> warnings = performanceWarningSystem->GetWarnings();
        if (!warnings.IsEmpty()) {
            int y = warningsRect.top;
            draw.DrawText(warningsRect.left, y, String("Performance Warnings:"), StdFont(), Red());
            y += lineHeight;
            
            for (const String& warning : warnings) {
                draw.DrawText(warningsRect.left, y, warning, StdFont(), Red());
                y += lineHeight;
            }
        }
    }
}

END_UPP_NAMESPACE