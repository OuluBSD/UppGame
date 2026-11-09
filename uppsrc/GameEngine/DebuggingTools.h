#ifndef UPP_DEBUGGING_TOOLS_H
#define UPP_DEBUGGING_TOOLS_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP

// FPS and memory tracker
class FpsMemoryTracker {
public:
    FpsMemoryTracker();
    virtual ~FpsMemoryTracker();

    // Initialize the tracker
    bool Initialize();

    // Update the tracker (call every frame)
    void Update();

    // Get current FPS
    double GetFPS() const { return currentFPS; }
    double GetAverageFPS() const { return averageFPS; }

    // Get memory usage
    int64 GetUsedMemory() const;
    int64 GetTotalMemory() const;
    double GetMemoryUsagePercent() const;

    // Reset statistics
    void Reset();

    // Get frame time statistics
    double GetFrameTimeMin() const { return frameTimeMin; }
    double GetFrameTimeMax() const { return frameTimeMax; }
    double GetFrameTimeAvg() const { return frameTimeAvg; }

private:
    // FPS tracking
    Vector<uint64> frameTimes;  // Last N frame times in milliseconds
    uint64 lastFrameTime = 0;
    double currentFPS = 0.0;
    double averageFPS = 0.0;
    int frameCount = 0;
    static const int MAX_FRAME_TIMES = 60;  // Track last 60 frames

    // Memory tracking
    int64 initialMemory = 0;
    int64 peakMemory = 0;

    // Frame time statistics
    double frameTimeMin = 1000.0;
    double frameTimeMax = 0.0;
    double frameTimeAvg = 0.0;
    Vector<double> frameTimeHistory;
};

// Frame analysis tools
class FrameAnalyzer {
public:
    FrameAnalyzer();
    virtual ~FrameAnalyzer();

    // Initialize the analyzer
    bool Initialize();

    // Begin frame analysis
    void BeginFrame();

    // End frame analysis
    void EndFrame();

    // Register rendering operations during the frame
    void RegisterDrawCall(int vertexCount, int triangleCount);
    void RegisterTextureBind(int textureId);
    void RegisterShaderBind(int shaderId);

    // Get frame statistics
    int GetDrawCallCount() const { return drawCallCount; }
    int GetTotalVertexCount() const { return totalVertexCount; }
    int GetTotalTriangleCount() const { return totalTriangleCount; }
    int GetTextureBindCount() const { return textureBindCount; }
    int GetShaderBindCount() const { return shaderBindCount; }

    // Reset frame statistics
    void Reset();

    // Get detailed timing breakdown
    double GetRenderTime() const { return renderTime; }
    double GetUpdateTime() const { return updateTime; }
    double GetOtherTime() const { return otherTime; }

private:
    // Frame statistics
    int drawCallCount = 0;
    int totalVertexCount = 0;
    int totalTriangleCount = 0;
    int textureBindCount = 0;
    int shaderBindCount = 0;

    // Timing
    uint64 frameStartTime = 0;
    uint64 renderStartTime = 0;
    uint64 updateStartTime = 0;
    double renderTime = 0.0;
    double updateTime = 0.0;
    double otherTime = 0.0;

    bool inFrame = false;
};

// Allocation profiler
class AllocationProfiler {
public:
    AllocationProfiler();
    virtual ~AllocationProfiler();

    // Initialize the profiler
    bool Initialize();

    // Track an allocation
    void TrackAllocation(void* ptr, size_t size, const String& source);

    // Track a deallocation
    void TrackDeallocation(void* ptr);

    // Get allocation statistics
    struct AllocationInfo {
        void* ptr;
        size_t size;
        String source;
        uint64 timestamp;
    };

    Vector<AllocationInfo> GetAllocations() const;
    size_t GetTotalAllocated() const { return totalAllocated; }
    size_t GetPeakAllocated() const { return peakAllocated; }
    int GetAllocationCount() const { return allocationCount; }

    // Reset profiler
    void Reset();

    // Enable/disable profiling
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

private:
    HashMap<void*, AllocationInfo> allocations;
    size_t totalAllocated = 0;
    size_t peakAllocated = 0;
    int allocationCount = 0;
    bool enabled = true;
    CriticalSection cs;  // Thread safety
};

// Performance warning system
class PerformanceWarningSystem {
public:
    PerformanceWarningSystem();
    virtual ~PerformanceWarningSystem();

    // Initialize the system
    bool Initialize();

    // Check for performance issues
    void CheckPerformance();

    // Register thresholds and warnings
    void SetHighFpsThreshold(double fps) { highFpsThreshold = fps; }
    void SetLowFpsThreshold(double fps) { lowFpsThreshold = fps; }
    void SetHighMemoryThreshold(int64 bytes) { highMemoryThreshold = bytes; }
    void SetHighDrawCallThreshold(int count) { highDrawCallThreshold = count; }

    // Get warnings
    Vector<String> GetWarnings() const { return warnings; }
    void ClearWarnings();

    // Add a performance metric to monitor
    void MonitorMetric(const String& name, double value, double threshold, 
                      const String& warningMessage);

    // Enable/disable the system
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

private:
    double highFpsThreshold = 200.0;      // Too high FPS (battery drain)
    double lowFpsThreshold = 30.0;        // Too low FPS (performance issue)
    int64 highMemoryThreshold = 1024 * 1024 * 500; // 500MB
    int highDrawCallThreshold = 1000;     // Too many draw calls

    Vector<String> warnings;
    bool enabled = true;

    struct MonitoredMetric {
        String name;
        double value;
        double threshold;
        String warningMessage;
    };
    Vector<MonitoredMetric> monitoredMetrics;
};

// Debugging tools UI
class DebuggingToolsUI {
public:
    DebuggingToolsUI();
    virtual ~DebuggingToolsUI();

    // Initialize the UI
    bool Initialize();

    // Update and render debugging UI
    void Update(double dt);
    void Render(Draw& draw, const Rect& viewport);

    // Show/hide the debugging UI
    void SetVisible(bool visible) { this->visible = visible; }
    bool IsVisible() const { return visible; }

    // Set which tools to display
    void SetShowFpsCounter(bool show) { showFpsCounter = show; }
    void SetShowMemoryUsage(bool show) { showMemoryUsage = show; }
    void SetShowFrameStats(bool show) { showFrameStats = show; }
    void SetShowPerformanceWarnings(bool show) { showPerformanceWarnings = show; }

    // Set the tools to display data from
    void SetFpsTracker(std::shared_ptr<FpsMemoryTracker> tracker) { fpsTracker = tracker; }
    void SetFrameAnalyzer(std::shared_ptr<FrameAnalyzer> analyzer) { frameAnalyzer = analyzer; }
    void SetPerformanceWarningSystem(std::shared_ptr<PerformanceWarningSystem> system) { 
        performanceWarningSystem = system; 
    }

private:
    bool visible = false;
    bool showFpsCounter = true;
    bool showMemoryUsage = true;
    bool showFrameStats = true;
    bool showPerformanceWarnings = true;

    std::shared_ptr<FpsMemoryTracker> fpsTracker;
    std::shared_ptr<FrameAnalyzer> frameAnalyzer;
    std::shared_ptr<PerformanceWarningSystem> performanceWarningSystem;

    // UI positioning and layout
    Rect fpsRect;
    Rect memoryRect;
    Rect statsRect;
    Rect warningsRect;
};

END_UPP_NAMESPACE

#endif