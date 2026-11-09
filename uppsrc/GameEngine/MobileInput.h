#ifndef UPP_MOBILE_INPUT_H
#define UPP_MOBILE_INPUT_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Vector/Vector.h>

NAMESPACE_UPP

// Represents 3-axis sensor data (accelerometer, gyroscope, etc.)
struct SensorData {
    double x, y, z;  // Sensor values along each axis
    Time timestamp;  // When the data was captured
    
    SensorData() : x(0), y(0), z(0), timestamp() {}
    SensorData(double x, double y, double z) : x(x), y(y), z(z), timestamp(GetSysTime()) {}
};

// Mobile sensor manager for handling device sensors
class MobileSensorManager {
public:
    MobileSensorManager();
    virtual ~MobileSensorManager();
    
    // Initialize sensor manager
    bool Initialize();
    
    // Check if sensors are available on this device
    bool IsSensorAvailable(const String& sensorType) const;
    
    // Start listening to a specific sensor
    bool StartSensor(const String& sensorType, double updateRateHz = 60.0);
    
    // Stop listening to a specific sensor
    bool StopSensor(const String& sensorType);
    
    // Get latest accelerometer data
    SensorData GetAccelerometerData() const;
    
    // Get latest gyroscope data
    SensorData GetGyroscopeData() const;
    
    // Get latest magnetometer data
    SensorData GetMagnetometerData() const;
    
    // Get latest rotation vector data (if available)
    SensorData GetRotationVectorData() const;
    
    // Get latest linear acceleration (acceleration minus gravity)
    SensorData GetLinearAccelerationData() const;
    
    // Get latest gravity vector
    SensorData GetGravityData() const;
    
    // Check if sensor data has been updated since last check
    bool HasSensorDataChanged(const String& sensorType) const;
    
    // Set calibration values for sensors
    void SetCalibration(const String& sensorType, const SensorData& calibration);
    
    // Get all available sensor types
    Vector<String> GetAvailableSensors() const;
    
    // Update method to be called regularly
    void Update();
    
    // Enable/disable the sensor manager
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

protected:
    bool enabled = false;
    
    // Latest sensor data
    SensorData accelerometerData;
    SensorData gyroscopeData;
    SensorData magnetometerData;
    SensorData rotationVectorData;
    SensorData linearAccelerationData;
    SensorData gravityData;
    
    // Sensor status tracking
    HashMap<String, bool> sensorStatus;
    
    // Last update tracking
    HashMap<String, Time> lastUpdate;
    HashMap<String, bool> dataChanged;
    
    // Calibration values
    HashMap<String, SensorData> calibration;
    
    // Platform-specific sensor reading methods (to be implemented)
    virtual bool PlatformStartSensor(const String& sensorType, double updateRateHz);
    virtual bool PlatformStopSensor(const String& sensorType);
    virtual SensorData PlatformGetSensorData(const String& sensorType);
    virtual bool PlatformIsSensorAvailable(const String& sensorType) const;
    virtual Vector<String> PlatformGetAvailableSensors() const;
};

// Mobile-specific input handling
class MobileInput {
public:
    MobileInput();
    virtual ~MobileInput();
    
    // Initialize mobile input system
    bool Initialize();
    
    // Handle touch events
    void TouchDown(int id, Point pos);
    void TouchUp(int id, Point pos);
    void TouchMove(int id, Point pos);
    void TouchCancel(int id, Point pos);
    
    // Multi-touch support
    int GetTouchCount() const { return touches.GetCount(); }
    Point GetTouchPos(int id) const;
    bool IsTouching(int id) const;
    Vector<Point> GetAllTouchPositions() const;
    
    // Gesture recognition
    class GestureRecognizer {
    public:
        virtual ~GestureRecognizer() = default;
        virtual bool ProcessTouchEvents(const VectorMap<int, Point>& touches) = 0;
        virtual bool IsGestureActive() const = 0;
        virtual void Reset() = 0;
    };
    
    // Add a gesture recognizer
    void AddGestureRecognizer(std::shared_ptr<GestureRecognizer> recognizer);
    
    // Swipe gesture recognizer
    class SwipeRecognizer : public GestureRecognizer {
    public:
        SwipeRecognizer(double minDistance = 50.0, double maxTimeMs = 500.0);
        virtual bool ProcessTouchEvents(const VectorMap<int, Point>& touches) override;
        virtual bool IsGestureActive() const override { return active; }
        virtual void Reset() override;
        
        enum class Direction { NONE, UP, DOWN, LEFT, RIGHT };
        Direction GetSwipeDirection() const { return direction; }
        double GetSwipeDistance() const { return distance; }
        
    private:
        double minDistance;
        double maxTimeMs;
        Time startTime;
        Point startPos;
        Point endPos;
        bool active;
        Direction direction;
        double distance;
    };
    
    // Pinch gesture recognizer for zooming
    class PinchRecognizer : public GestureRecognizer {
    public:
        PinchRecognizer(double minDistance = 20.0);
        virtual bool ProcessTouchEvents(const VectorMap<int, Point>& touches) override;
        virtual bool IsGestureActive() const override { return active; }
        virtual void Reset() override;
        
        double GetScaleFactor() const { return scaleFactor; }
        Point GetFocusPoint() const { return focusPoint; }
        
    private:
        double minDistance;
        Point prevPos1, prevPos2;
        Point currPos1, currPos2;
        double initialDistance;
        double scaleFactor;
        Point focusPoint;
        bool active;
        bool started;
    };
    
    // Update input system (call every frame)
    void Update();
    
    // Enable/disable mobile input
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

private:
    bool enabled = false;
    
    // Active touches
    VectorMap<int, Point> touches;
    VectorMap<int, Point> prevTouches;
    
    // Gesture recognizers
    Vector<std::shared_ptr<GestureRecognizer>> gestureRecognizers;
    
    // Update tracking
    Time lastUpdate;
};

END_UPP_NAMESPACE

#endif