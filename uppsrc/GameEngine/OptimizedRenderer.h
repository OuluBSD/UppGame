#ifndef UPP_OPTIMIZED_RENDERER_H
#define UPP_OPTIMIZED_RENDERER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/RenderBatching.h>
#include <GameEngine/SpriteMesh.h>
#include <GameEngine/Lighting.h>

NAMESPACE_UPP

// Optimized renderer for efficient 3D rendering
class OptimizedRenderer {
public:
    OptimizedRenderer();
    virtual ~OptimizedRenderer();

    // Initialize the optimized renderer
    bool Initialize();

    // Add objects to be rendered
    void AddMesh(const Mesh& mesh, const Matrix4& transform, 
                 std::shared_ptr<Material> material = nullptr);
    void AddSprite(const Sprite& sprite, const Matrix4& transform,
                   std::shared_ptr<Material> material = nullptr);

    // Add 3D object with more complex parameters
    void Add3DObject(const Mesh& mesh, 
                     const Matrix4& worldTransform,
                     const Matrix4& viewProjection,
                     std::shared_ptr<Material> material,
                     const Color& color = White());

    // Render all queued objects
    void Render(Draw& draw, const Rect& viewport);

    // Clear all queued objects
    void Clear();

    // Get performance statistics
    int GetDrawCallCount() const { return drawCallCount; }
    int GetVertexCount() const { return vertexCount; }
    int GetTriangleCount() const { return triangleCount; }

    // Frustum culling
    void SetFrustumCullingEnabled(bool enabled) { frustumCulling = enabled; }
    bool IsFrustumCullingEnabled() const { return frustumCulling; }

    // Occlusion culling
    void SetOcclusionCullingEnabled(bool enabled) { occlusionCulling = enabled; }
    bool IsOcclusionCullingEnabled() const { return occlusionCulling; }

    // Level of Detail (LOD) system
    void SetLODEnabled(bool enabled) { lodEnabled = enabled; }
    bool IsLODEnabled() const { return lodEnabled; }

    void SetLODDistance(double distance) { lodDistance = distance; }
    double GetLODDistance() const { return lodDistance; }

    // Set rendering quality level
    enum class QualityLevel { 
        LOW, MEDIUM, HIGH, ULTRA 
    };
    void SetQualityLevel(QualityLevel level) { qualityLevel = level; }
    QualityLevel GetQualityLevel() const { return qualityLevel; }

    // Lighting optimization
    void SetLightingOptimizationEnabled(bool enabled) { lightingOptimization = enabled; }
    bool IsLightingOptimizationEnabled() const { return lightingOptimization; }

    // Set the lighting system for optimization
    void SetLightingSystem(std::shared_ptr<LightingSystem> lighting) { 
        this->lightingSystem = lighting; 
    }

    // Get/set camera for view frustum culling
    void SetCameraPosition(const Point3& pos) { cameraPosition = pos; }
    Point3 GetCameraPosition() const { return cameraPosition; }

private:
    // Rendering queues
    struct RenderCommand {
        enum class Type { MESH, SPRITE, CUSTOM };
        Type type;
        Mesh mesh;
        Sprite sprite;
        Matrix4 transform;
        Matrix4 viewProjection;
        std::shared_ptr<Material> material;
        Color color;
        Rect3 bounds;  // Bounding box for culling
    };

    Vector<RenderCommand> renderQueue;
    
    // Performance counters
    int drawCallCount = 0;
    int vertexCount = 0;
    int triangleCount = 0;
    
    // Optimization settings
    bool frustumCulling = true;
    bool occlusionCulling = false;
    bool lodEnabled = true;
    double lodDistance = 50.0;
    QualityLevel qualityLevel = QualityLevel::HIGH;
    bool lightingOptimization = true;
    
    // Systems
    std::shared_ptr<LightingSystem> lightingSystem;
    Point3 cameraPosition = Point3(0, 0, 0);
    
    // View frustum for culling
    // In a real implementation, we would store the actual frustum planes
    // For now, we'll just track the camera position and implement simple distance culling
    
    // Frustum culling helper
    bool IsInFrustum(const Rect3& bounds) const;
    
    // Distance-based culling
    bool IsWithinRenderDistance(const Rect3& bounds) const;
    
    // LOD selection
    int SelectLOD(const Rect3& bounds) const;
    
    // Batch similar objects for fewer draw calls
    void BatchRenderCommands();
};

// Instance rendering for rendering many similar objects efficiently
class InstanceRenderer {
public:
    InstanceRenderer();
    virtual ~InstanceRenderer();

    // Add an object to be rendered multiple times
    void AddInstance(const Mesh& mesh, const Vector<Matrix4>& transforms,
                     std::shared_ptr<Material> material = nullptr);

    // Render all instances
    void Render(Draw& draw, const Matrix4& viewProjection, const Rect& viewport);

    // Get/set maximum instances per batch
    void SetMaxInstancesPerBatch(int max) { maxInstancesPerBatch = max; }
    int GetMaxInstancesPerBatch() const { return maxInstancesPerBatch; }

private:
    struct InstanceBatch {
        Mesh mesh;
        Vector<Matrix4> transforms;
        std::shared_ptr<Material> material;
    };

    Vector<InstanceBatch> batches;
    int maxInstancesPerBatch = 1000;  // Maximum instances per batch
    
    void CreateBatches();
};

END_UPP_NAMESPACE

#endif