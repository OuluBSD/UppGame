#include "MobileInput.h"

NAMESPACE_UPP

MobileSensorManager::MobileSensorManager() {
}

MobileSensorManager::~MobileSensorManager() {
}

bool MobileSensorManager::Initialize() {
    enabled = true;
    return true;
}

bool MobileSensorManager::IsSensorAvailable(const String& sensorType) const {
    return PlatformIsSensorAvailable(sensorType);
}

bool MobileSensorManager::StartSensor(const String& sensorType, double updateRateHz) {
    if (!enabled) return false;
    bool result = PlatformStartSensor(sensorType, updateRateHz);
    if (result) {
        sensorStatus.GetAdd(sensorType) = true;
    }
    return result;
}

bool MobileSensorManager::StopSensor(const String& sensorType) {
    if (!enabled) return false;
    bool result = PlatformStopSensor(sensorType);
    if (result) {
        sensorStatus.GetAdd(sensorType) = false;
    }
    return result;
}

SensorData MobileSensorManager::GetAccelerometerData() const {
    return accelerometerData;
}

SensorData MobileSensorManager::GetGyroscopeData() const {
    return gyroscopeData;
}

SensorData MobileSensorManager::GetMagnetometerData() const {
    return magnetometerData;
}

SensorData MobileSensorManager::GetRotationVectorData() const {
    return rotationVectorData;
}

SensorData MobileSensorManager::GetLinearAccelerationData() const {
    return linearAccelerationData;
}

SensorData MobileSensorManager::GetGravityData() const {
    return gravityData;
}

bool MobileSensorManager::HasSensorDataChanged(const String& sensorType) const {
    auto it = dataChanged.Find(sensorType);
    if (it != dataChanged.End()) {
        return it->value;
    }
    return false;
}

void MobileSensorManager::SetCalibration(const String& sensorType, const SensorData& calibration) {
    this->calibration.GetAdd(sensorType) = calibration;
}

Vector<String> MobileSensorManager::GetAvailableSensors() const {
    return PlatformGetAvailableSensors();
}

void MobileSensorManager::Update() {
    if (!enabled) return;
    
    // For each active sensor, try to get new data
    for (auto& pair : sensorStatus) {
        if (pair.value) {  // If sensor is active
            SensorData data = PlatformGetSensorData(pair.key);
            if (data.timestamp != lastUpdate.Get(pair.key)) {
                // Update last update time
                lastUpdate.GetAdd(pair.key) = data.timestamp;
                
                // Apply calibration if available
                auto calIt = calibration.Find(pair.key);
                if (calIt != calibration.End()) {
                    data.x -= calIt->value.x;
                    data.y -= calIt->value.y;
                    data.z -= calIt->value.z;
                }
                
                // Store the data based on sensor type
                if (pair.key == "accelerometer") {
                    accelerometerData = data;
                } else if (pair.key == "gyroscope") {
                    gyroscopeData = data;
                } else if (pair.key == "magnetometer") {
                    magnetometerData = data;
                } else if (pair.key == "rotation_vector") {
                    rotationVectorData = data;
                } else if (pair.key == "linear_acceleration") {
                    linearAccelerationData = data;
                } else if (pair.key == "gravity") {
                    gravityData = data;
                }
                
                // Mark that data has changed
                dataChanged.GetAdd(pair.key) = true;
            } else {
                // Mark that data has not changed
                dataChanged.GetAdd(pair.key) = false;
            }
        }
    }
}

// Default implementations for platform-specific methods (return defaults)
bool MobileSensorManager::PlatformStartSensor(const String& sensorType, double updateRateHz) {
    // Default implementation - just return true to indicate success
    // In a real implementation, this would interface with platform APIs
    return true;
}

bool MobileSensorManager::PlatformStopSensor(const String& sensorType) {
    // Default implementation
    return true;
}

SensorData MobileSensorManager::PlatformGetSensorData(const String& sensorType) {
    // Default implementation returns zero data
    // In a real implementation, this would get data from platform APIs
    return SensorData(0, 0, 0);
}

bool MobileSensorManager::PlatformIsSensorAvailable(const String& sensorType) const {
    // Default implementation assumes no sensors are available
    return false;
}

Vector<String> MobileSensorManager::PlatformGetAvailableSensors() const {
    // Default implementation returns empty vector
    return Vector<String>();
}

// MobileInput implementation
MobileInput::MobileInput() {
}

MobileInput::~MobileInput() {
}

bool MobileInput::Initialize() {
    enabled = true;
    lastUpdate = GetSysTime();
    return true;
}

void MobileInput::TouchDown(int id, Point pos) {
    if (!enabled) return;
    prevTouches.GetAdd(id) = touches.Get(id, pos);
    touches.GetAdd(id) = pos;
}

void MobileInput::TouchUp(int id, Point pos) {
    if (!enabled) return;
    prevTouches.GetAdd(id) = touches.Get(id, pos);
    touches.RemoveKey(id);
}

void MobileInput::TouchMove(int id, Point pos) {
    if (!enabled) return;
    prevTouches.GetAdd(id) = touches.Get(id, pos);
    touches.GetAdd(id) = pos;
}

void MobileInput::TouchCancel(int id, Point pos) {
    if (!enabled) return;
    touches.RemoveKey(id);
    prevTouches.RemoveKey(id);
}

Point MobileInput::GetTouchPos(int id) const {
    return touches.Get(id, Point(0, 0));
}

bool MobileInput::IsTouching(int id) const {
    return touches.Find(id) >= 0;
}

Vector<Point> MobileInput::GetAllTouchPositions() const {
    Vector<Point> positions;
    for (const auto& pair : touches) {
        positions.Add(pair.value);
    }
    return positions;
}

void MobileInput::AddGestureRecognizer(std::shared_ptr<GestureRecognizer> recognizer) {
    gestureRecognizers.Add(recognizer);
}

void MobileInput::Update() {
    if (!enabled) return;
    
    Time now = GetSysTime();
    
    // Process all gesture recognizers with current touch data
    for (auto& recognizer : gestureRecognizers) {
        recognizer->ProcessTouchEvents(touches);
    }
    
    // Update previous touches for next frame
    prevTouches.Clear();
    for (const auto& pair : touches) {
        prevTouches.GetAdd(pair.key) = pair.value;
    }
    
    lastUpdate = now;
}

// SwipeRecognizer implementation
MobileInput::SwipeRecognizer::SwipeRecognizer(double minDistance, double maxTimeMs)
    : minDistance(minDistance), maxTimeMs(maxTimeMs), active(false), direction(Direction::NONE), distance(0) {
}

bool MobileInput::SwipeRecognizer::ProcessTouchEvents(const VectorMap<int, Point>& touches) {
    // Only process single touch swipes
    if (touches.GetCount() != 1) {
        if (active) {
            Reset();
        }
        return false;
    }
    
    auto it = touches.Begin();
    int touchId = it->key;
    Point currentPos = it->value;
    
    if (!active) {
        // Starting a new swipe
        startPos = currentPos;
        startTime = GetSysTime();
        active = true;
        direction = Direction::NONE;
        return false;
    }
    
    // Calculate distance from start
    double dx = currentPos.x - startPos.x;
    double dy = currentPos.y - startPos.y;
    distance = sqrt(dx*dx + dy*dy);
    
    // Check if enough distance has been covered
    if (distance >= minDistance) {
        // Determine direction based on greatest movement
        if (abs(dx) > abs(dy)) {
            direction = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
        } else {
            direction = (dy > 0) ? Direction::DOWN : Direction::UP;
        }
        
        Reset(); // Reset after recognizing swipe
        return true;
    }
    
    // Check if too much time has passed
    Time now = GetSysTime();
    double elapsedMs = (now - startTime).GetDouble();
    if (elapsedMs > maxTimeMs) {
        Reset();
        return false;
    }
    
    return false;
}

void MobileInput::SwipeRecognizer::Reset() {
    active = false;
    direction = Direction::NONE;
    distance = 0;
}

// PinchRecognizer implementation
MobileInput::PinchRecognizer::PinchRecognizer(double minDistance)
    : minDistance(minDistance), scaleFactor(1.0), active(false), started(false) {
}

bool MobileInput::PinchRecognizer::ProcessTouchEvents(const VectorMap<int, Point>& touches) {
    // Need exactly 2 touches for pinch
    if (touches.GetCount() != 2) {
        if (active) {
            Reset();
        }
        return false;
    }
    
    auto it = touches.Begin();
    Point pos1 = it->value;
    ++it;
    Point pos2 = it->value;
    
    double currentDistance = sqrt(pow(pos2.x - pos1.x, 2) + pow(pos2.y - pos1.y, 2));
    
    if (!started) {
        // Initialize pinch recognition
        prevPos1 = pos1;
        prevPos2 = pos2;
        currPos1 = pos1;
        currPos2 = pos2;
        initialDistance = currentDistance;
        started = true;
        active = true;
        scaleFactor = 1.0;
        return false;
    }
    
    // Update positions
    prevPos1 = currPos1;
    prevPos2 = currPos2;
    currPos1 = pos1;
    currPos2 = pos2;
    
    // Calculate new scale factor
    double newScale = currentDistance / initialDistance;
    if (abs(newScale - 1.0) > 0.1) {  // Only consider significant changes
        scaleFactor = newScale;
        
        // Calculate focus point (center of pinch)
        focusPoint.x = (pos1.x + pos2.x) / 2;
        focusPoint.y = (pos1.y + pos2.y) / 2;
        
        return true;
    }
    
    return false;
}

void MobileInput::PinchRecognizer::Reset() {
    started = false;
    active = false;
    scaleFactor = 1.0;
    focusPoint = Point(0, 0);
}

END_UPP_NAMESPACE