#ifndef UPP_RENDER_BATCHING_H
#define UPP_RENDER_BATCHING_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP_BEGIN

// Render batching strategies
enum class BatchingStrategy {
    BY_MATERIAL,      // Batch by material/texture
    BY_SHADER,        // Batch by shader program
    BY_GEOMETRY,      // Batch by geometry/mesh
    BY_DISTANCE,      // Batch by distance from camera
    BY_LAYER,         // Batch by render layer
    AUTO             // Automatically choose best strategy
};

// Render command types
enum class RenderCommandType {
    MESH,
    SPRITE,
    LINE,
    POINT,
    CUSTOM
};

// Base render command
struct RenderCommand {
    RenderCommandType type;
    SharedPtr<Material> material;
    Matrix4 transform;
    int layer = 0;
    double distance = 0.0;  // Distance from camera for sorting
    bool transparent = false;  // Whether this should be rendered after opaque objects
    void* data = nullptr;      // Pointer to specific data (mesh, sprite, etc.)
    
    RenderCommand() : type(RenderCommandType::CUSTOM), layer(0), distance(0.0), transparent(false), data(nullptr) {}
};

// Render batch - a collection of render commands that can be rendered together
class RenderBatch {
public:
    RenderBatch();
    ~RenderBatch();
    
    // Add a render command to this batch
    void AddCommand(const RenderCommand& command);
    
    // Clear all commands from this batch
    void Clear();
    
    // Check if batch is empty
    bool IsEmpty() const { return commands.empty(); }
    
    // Get command count
    int GetCommandCount() const { return (int)commands.size(); }
    
    // Get total memory used by this batch
    size_t GetMemoryUsage() const;
    
    // Get the shared material for this batch (all commands should use same material for efficiency)
    SharedPtr<Material> GetSharedMaterial() const;
    
    // Get the common shader used by this batch
    SharedPtr<ShaderProgram> GetSharedShader() const;
    
    // Execute all commands in this batch
    void Execute(Draw& draw) const;
    
    // Sort commands by distance (for transparency/depth sorting)
    void SortByDistance(bool descending = false);
    
    // Sort commands by layer
    void SortByLayer();
    
    // Get bounding box of all objects in batch (approximate)
    Rect3 GetBoundingBox() const;
    
private:
    Vector<RenderCommand> commands;
};

// Render batch manager - organizes render commands into efficient batches
class RenderBatchManager {
public:
    RenderBatchManager();
    ~RenderBatchManager();
    
    // Initialize the batch manager
    void Initialize();
    
    // Add a render command for batching
    void AddRenderCommand(const RenderCommand& command);
    
    // Batch all pending commands based on strategy
    void BatchCommands(BatchingStrategy strategy = BatchingStrategy::AUTO);
    
    // Execute all batches (call once per frame)
    void ExecuteBatches(Draw& draw);
    
    // Clear all commands and batches
    void Clear();
    
    // Get batch count
    int GetBatchCount() const { return (int)batches.size(); }
    
    // Get command count
    int GetCommandCount() const { return (int)pending_commands.size(); }
    
    // Get total memory usage
    size_t GetMemoryUsage() const;
    
    // Set maximum batch size (number of commands per batch)
    void SetMaxBatchSize(int size) { max_batch_size = size; }
    int GetMaxBatchSize() const { return max_batch_size; }
    
    // Set whether to sort transparent objects
    void SetSortTransparent(bool sort) { sort_transparent = sort; }
    bool GetSortTransparent() const { return sort_transparent; }
    
    // Set whether to use depth testing
    void SetDepthTesting(bool enable) { depth_testing = enable; }
    bool GetDepthTesting() const { return depth_testing; }
    
    // Get batching statistics
    struct BatchStats {
        int total_commands;
        int total_batches;
        int max_batch_size;
        int avg_batch_size;
        size_t memory_usage;
    };
    
    BatchStats GetStats() const;
    
    // Optimize batching - combine small batches when possible
    void Optimize();
    
    // Get batches for debugging/analysis
    const Vector<RenderBatch>& GetBatches() const { return batches; }
    
private:
    // Internal batching methods
    void BatchByMaterial();
    void BatchByShader();
    void BatchByGeometry();
    void BatchByDistance();
    void BatchByLayer();
    void BatchAuto();
    
    // Data
    Vector<RenderCommand> pending_commands;
    Vector<RenderBatch> batches;
    int max_batch_size = 100;  // Max commands per batch
    bool sort_transparent = true;
    bool depth_testing = true;
};

// Simple renderer that uses batching internally
class BatchedRenderer {
public:
    BatchedRenderer();
    ~BatchedRenderer();
    
    // Initialize the batched renderer
    void Initialize();
    
    // Add a mesh to be rendered (will be batched)
    void AddMesh(const Mesh& mesh, const Matrix4& transform, SharedPtr<Material> material, 
                 int layer = 0, bool transparent = false);
    
    // Add a sprite to be rendered (will be batched)
    void AddSprite(const Sprite& sprite, const Matrix4& transform, SharedPtr<Material> material,
                   int layer = 0, bool transparent = false);
    
    // Add a custom render command
    void AddRenderCommand(const RenderCommand& command);
    
    // Render all objects using optimized batching
    void Render(Draw& draw, BatchingStrategy strategy = BatchingStrategy::AUTO);
    
    // Clear all pending objects
    void Clear();
    
    // Get stats from the internal batch manager
    RenderBatchManager::BatchStats GetStats() const;
    
    // Set batching parameters
    void SetMaxBatchSize(int size) { batch_manager.SetMaxBatchSize(size); }
    void SetSortTransparent(bool sort) { batch_manager.SetSortTransparent(sort); }
    void SetDepthTesting(bool enable) { batch_manager.SetDepthTesting(enable); }
    
private:
    RenderBatchManager batch_manager;
    Vector<Mesh> meshes;
    Vector<Sprite> sprites;
    Vector<RenderCommand> temp_commands;  // Temporary storage for conversion
};

// Implementation
inline RenderBatch::RenderBatch() {
    // Initialize the render batch
}

inline RenderBatch::~RenderBatch() {
    // Clean up render batch
}

inline void RenderBatch::AddCommand(const RenderCommand& command) {
    commands.Add(command);
}

inline void RenderBatch::Clear() {
    commands.Clear();
}

inline size_t RenderBatch::GetMemoryUsage() const {
    // Rough estimate - in a real implementation, this would be more precise
    return commands.GetCount() * sizeof(RenderCommand);
}

inline SharedPtr<Material> RenderBatch::GetSharedMaterial() const {
    if (commands.IsEmpty()) return nullptr;
    return commands[0].material;
}

inline SharedPtr<ShaderProgram> RenderBatch::GetSharedShader() const {
    auto material = GetSharedMaterial();
    return material ? material->GetShader() : nullptr;
}

inline void RenderBatch::Execute(Draw& draw) const {
    for (const auto& command : commands) {
        // In a real implementation, this would execute the specific rendering command
        // For this basic implementation, we'll just draw a simple shape based on type
        switch (command.type) {
            case RenderCommandType::SPRITE: {
                // Draw a simple rectangle for sprite
                Size sz = draw.GetSize();
                draw.DrawRect(0, 0, 100, 100, Color(100, 150, 200));
                break;
            }
            case RenderCommandType::MESH: {
                // Draw a simple shape for mesh
                Size sz = draw.GetSize();
                draw.DrawRect(0, 0, 50, 50, Color(200, 100, 150));
                break;
            }
            default: {
                // Draw default shape
                Size sz = draw.GetSize();
                draw.DrawRect(0, 0, 30, 30, White());
                break;
            }
        }
    }
}

inline void RenderBatch::SortByDistance(bool descending) {
    if (descending) {
        Sort(commands, [](const RenderCommand& a, const RenderCommand& b) {
            return a.distance > b.distance;
        });
    } else {
        Sort(commands, [](const RenderCommand& a, const RenderCommand& b) {
            return a.distance < b.distance;
        });
    }
}

inline void RenderBatch::SortByLayer() {
    Sort(commands, [](const RenderCommand& a, const RenderCommand& b) {
        return a.layer < b.layer;
    });
}

inline Rect3 RenderBatch::GetBoundingBox() const {
    if (commands.IsEmpty()) {
        return Rect3(Point3(0, 0, 0), Point3(0, 0, 0));
    }
    
    // For simplicity, just return a default bounding box
    // In a real implementation, this would calculate from the actual geometry
    return Rect3(Point3(-10, -10, -10), Point3(10, 10, 10));
}

// RenderBatchManager implementation
inline RenderBatchManager::RenderBatchManager() {
    // Initialize the batch manager
}

inline RenderBatchManager::~RenderBatchManager() {
    Clear();
}

inline void RenderBatchManager::Initialize() {
    Clear();
}

inline void RenderBatchManager::AddRenderCommand(const RenderCommand& command) {
    pending_commands.Add(command);
}

inline void RenderBatchManager::BatchCommands(BatchingStrategy strategy) {
    Clear();
    
    if (pending_commands.IsEmpty()) return;
    
    switch (strategy) {
        case BatchingStrategy::BY_MATERIAL:
            BatchByMaterial();
            break;
        case BatchingStrategy::BY_SHADER:
            BatchByShader();
            break;
        case BatchingStrategy::BY_GEOMETRY:
            BatchByGeometry();
            break;
        case BatchingStrategy::BY_DISTANCE:
            BatchByDistance();
            break;
        case BatchingStrategy::BY_LAYER:
            BatchByLayer();
            break;
        case BatchingStrategy::AUTO:
        default:
            BatchAuto();
            break;
    }
    
    // Sort transparent objects if enabled
    if (sort_transparent) {
        for (auto& batch : batches) {
            if (batch.GetCommandCount() > 0 && pending_commands[0].transparent) {
                batch.SortByDistance(true); // Sort by descending distance for transparency
            }
        }
    }
}

inline void RenderBatchManager::ExecuteBatches(Draw& draw) {
    for (auto& batch : batches) {
        if (!batch.IsEmpty()) {
            batch.Execute(draw);
        }
    }
}

inline void RenderBatchManager::Clear() {
    pending_commands.Clear();
    batches.Clear();
}

inline size_t RenderBatchManager::GetMemoryUsage() const {
    size_t total = 0;
    for (const auto& batch : batches) {
        total += batch.GetMemoryUsage();
    }
    total += pending_commands.GetCount() * sizeof(RenderCommand);
    return total;
}

inline RenderBatchManager::BatchStats RenderBatchManager::GetStats() const {
    BatchStats stats = {};
    stats.total_commands = GetCommandCount();
    stats.total_batches = GetBatchCount();
    stats.max_batch_size = max_batch_size;
    
    if (stats.total_batches > 0) {
        stats.avg_batch_size = stats.total_commands / stats.total_batches;
    }
    
    stats.memory_usage = GetMemoryUsage();
    
    return stats;
}

inline void RenderBatchManager::Optimize() {
    // In a real implementation, this would try to combine small batches
    // For now, it's a placeholder
}

inline void RenderBatchManager::BatchByMaterial() {
    // Group commands by material
    std::map<Material*, Vector<RenderCommand>> material_groups;
    
    for (const auto& cmd : pending_commands) {
        Material* mat = cmd.material.get();
        material_groups[mat].Add(cmd);
    }
    
    // Create batches for each material group
    for (const auto& group : material_groups) {
        RenderBatch batch;
        for (const auto& cmd : group.second) {
            batch.AddCommand(cmd);
            if (batch.GetCommandCount() >= max_batch_size) {
                batches.Add(batch);
                batch.Clear();
            }
        }
        if (!batch.IsEmpty()) {
            batches.Add(batch);
        }
    }
}

inline void RenderBatchManager::BatchByShader() {
    // Group commands by shader
    std::map<ShaderProgram*, Vector<RenderCommand>> shader_groups;
    
    for (const auto& cmd : pending_commands) {
        ShaderProgram* shader = cmd.material ? cmd.material->GetShader().get() : nullptr;
        shader_groups[shader].Add(cmd);
    }
    
    // Create batches for each shader group
    for (const auto& group : shader_groups) {
        RenderBatch batch;
        for (const auto& cmd : group.second) {
            batch.AddCommand(cmd);
            if (batch.GetCommandCount() >= max_batch_size) {
                batches.Add(batch);
                batch.Clear();
            }
        }
        if (!batch.IsEmpty()) {
            batches.Add(batch);
        }
    }
}

inline void RenderBatchManager::BatchByLayer() {
    // Group commands by layer
    std::map<int, Vector<RenderCommand>> layer_groups;
    
    for (const auto& cmd : pending_commands) {
        layer_groups[cmd.layer].Add(cmd);
    }
    
    // Create batches for each layer group
    for (const auto& group : layer_groups) {
        RenderBatch batch;
        for (const auto& cmd : group.second) {
            batch.AddCommand(cmd);
            if (batch.GetCommandCount() >= max_batch_size) {
                batches.Add(batch);
                batch.Clear();
            }
        }
        if (!batch.IsEmpty()) {
            batches.Add(batch);
        }
    }
}

inline void RenderBatchManager::BatchAuto() {
    // For this implementation, just use material-based batching as default
    BatchByMaterial();
}

// BatchedRenderer implementation
inline BatchedRenderer::BatchedRenderer() {
    // Initialize the batched renderer
}

inline BatchedRenderer::~BatchedRenderer() {
    // Clean up
}

inline void BatchedRenderer::Initialize() {
    batch_manager.Initialize();
}

inline void BatchedRenderer::AddMesh(const Mesh& mesh, const Matrix4& transform, 
                                     SharedPtr<Material> material, int layer, bool transparent) {
    // In a full implementation, we'd store the mesh and convert to render commands
    // For now, we'll just create a placeholder command
    RenderCommand cmd;
    cmd.type = RenderCommandType::MESH;
    cmd.material = material;
    cmd.transform = transform;
    cmd.layer = layer;
    cmd.transparent = transparent;
    cmd.data = (void*)&mesh;  // In a real implementation, this would be handled differently
    
    batch_manager.AddRenderCommand(cmd);
}

inline void BatchedRenderer::AddSprite(const Sprite& sprite, const Matrix4& transform,
                                       SharedPtr<Material> material, int layer, bool transparent) {
    // Similar to AddMesh, but for sprites
    RenderCommand cmd;
    cmd.type = RenderCommandType::SPRITE;
    cmd.material = material;
    cmd.transform = transform;
    cmd.layer = layer;
    cmd.transparent = transparent;
    cmd.data = (void*)&sprite;  // In a real implementation, this would be handled differently
    
    batch_manager.AddRenderCommand(cmd);
}

inline void BatchedRenderer::AddRenderCommand(const RenderCommand& command) {
    batch_manager.AddRenderCommand(command);
}

inline void BatchedRenderer::Render(Draw& draw, BatchingStrategy strategy) {
    batch_manager.BatchCommands(strategy);
    batch_manager.ExecuteBatches(draw);
    batch_manager.Clear();  // Clear after rendering
}

inline void BatchedRenderer::Clear() {
    batch_manager.Clear();
}

inline RenderBatchManager::BatchStats BatchedRenderer::GetStats() const {
    return batch_manager.GetStats();
}

NAMESPACE_UPP_END

#endif