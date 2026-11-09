#ifndef UPP_SHADER_H
#define UPP_SHADER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP_BEGIN

// Shader base class representing a single shader stage
class ShaderStage {
public:
    enum Type {
        VERTEX_SHADER,
        FRAGMENT_SHADER,
        GEOMETRY_SHADER,
        COMPUTE_SHADER
    };
    
    ShaderStage() = default;
    virtual ~ShaderStage() = default;
    
    // Load shader from source string
    virtual bool LoadFromSource(const String& source, Type type) = 0;
    
    // Load shader from file
    virtual bool LoadFromFile(const String& filepath, Type type) = 0;
    
    // Get shader type
    Type GetType() const { return type; }
    
    // Check if shader is loaded
    bool IsLoaded() const { return loaded; }
    
    // Get shader ID (platform-specific)
    virtual int GetPlatformId() const = 0;
    
protected:
    Type type;
    bool loaded = false;
    String source_code;
};

// Shader program combining multiple shader stages
class ShaderProgram {
public:
    ShaderProgram() = default;
    virtual ~ShaderProgram() = default;
    
    // Add a shader stage to the program
    virtual bool AddShaderStage(const ShaderStage& stage) = 0;
    
    // Remove all shader stages
    virtual void ClearShaders() = 0;
    
    // Link the program
    virtual bool Link() = 0;
    
    // Use this program for rendering
    virtual void Use() = 0;
    
    // Get uniform location by name
    virtual int GetUniformLocation(const String& name) = 0;
    
    // Set uniform values (various types)
    virtual void SetUniform(const String& name, int value) = 0;
    virtual void SetUniform(const String& name, double value) = 0;
    virtual void SetUniform(const String& name, float value) = 0;
    virtual void SetUniform(const String& name, const Point3& value) = 0;
    virtual void SetUniform(const String& name, const Vector3& value) = 0;
    virtual void SetUniform(const String& name, const Matrix4& value) = 0;
    virtual void SetUniform(const String& name, const Color& value) = 0;
    
    // Check if program is linked and valid
    bool IsValid() const { return linked; }
    
    // Get program ID (platform-specific) 
    virtual int GetPlatformId() const = 0;
    
protected:
    bool linked = false;
    Vector<ShaderStage> shaders;
};

// Shader manager for loading, caching and managing shader programs
class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();
    
    // Load a shader program from files
    SharedPtr<ShaderProgram> LoadProgram(const String& vertexFile, const String& fragmentFile, 
                                        const String& geometryFile = String(), 
                                        const String& name = String());
    
    // Load a shader program from source strings
    SharedPtr<ShaderProgram> LoadProgramFromSource(const String& vertexSource, 
                                                  const String& fragmentSource,
                                                  const String& geometrySource = String(),
                                                  const String& name = String());
    
    // Get a shader program by name
    SharedPtr<ShaderProgram> GetProgram(const String& name);
    const SharedPtr<ShaderProgram> GetProgram(const String& name) const;
    
    // Unload a specific shader program
    void UnloadProgram(const String& name);
    
    // Unload all shader programs
    void UnloadAllPrograms();
    
    // Check if a shader program exists
    bool HasProgram(const String& name) const;
    
    // Get/set memory budget for shaders (in bytes)
    void SetMemoryBudget(uint64 budget) { memory_budget = budget; }
    uint64 GetMemoryBudget() const { return memory_budget; }
    uint64 GetUsedMemory() const { return used_memory; }
    
private:
    // Map of shader names to shader programs
    std::map<String, SharedPtr<ShaderProgram>> programs;
    
    // Memory tracking
    uint64 memory_budget = 100 * 1024 * 1024; // 100MB default
    uint64 used_memory = 0;
    
    // Helper to estimate shader memory usage
    size_t EstimateShaderSize(const ShaderProgram& program) const;
    
    // Generate a unique name if none provided
    String GenerateUniqueName(const String& baseName = "shader");
    int name_counter = 0;
};

// Basic shader implementation that works with the U++ Draw system
// This is a simplified implementation that provides shader-like functionality
// using U++ drawing capabilities until actual GPU shader support is available
class BasicShaderProgram : public ShaderProgram {
public:
    BasicShaderProgram();
    
    // Add a shader stage to the program
    bool AddShaderStage(const ShaderStage& stage) override;
    
    // Remove all shader stages
    void ClearShaders() override;
    
    // Link the program
    bool Link() override;
    
    // Use this program for rendering
    void Use() override;
    
    // Get uniform location by name
    int GetUniformLocation(const String& name) override;
    
    // Set uniform values (various types)
    void SetUniform(const String& name, int value) override;
    void SetUniform(const String& name, double value) override;
    void SetUniform(const String& name, float value) override;
    void SetUniform(const String& name, const Point3& value) override;
    void SetUniform(const String& name, const Vector3& value) override;
    void SetUniform(const String& name, const Matrix4& value) override;
    void SetUniform(const String& name, const Color& value) override;
    
    // Get program ID (platform-specific) 
    int GetPlatformId() const override { return platform_id; }
    
    // Apply shader effects to a draw operation
    void ApplyEffect(Draw& draw, const std::function<void(Draw&)>& renderFunc);
    
private:
    int platform_id = 0;
    std::map<String, Value> uniforms;  // Store uniforms by name
    bool uniforms_dirty = true;        // Flag when uniforms change
    
    // For this basic implementation, we'll use the uniforms to affect drawing operations
    Color GetColorFromUniforms() const;
    double GetScalarFromUniforms(const String& name, double defaultValue = 1.0) const;
};

// Implementation
inline ShaderManager::ShaderManager() {
    // Initialize the shader manager
}

inline ShaderManager::~ShaderManager() {
    UnloadAllPrograms();
}

inline SharedPtr<ShaderProgram> ShaderManager::LoadProgram(const String& vertexFile, 
                                                          const String& fragmentFile,
                                                          const String& geometryFile,
                                                          const String& name) {
    // For now, we'll create a basic shader program
    // In a real implementation, this would load from actual shader files
    String progName = name.IsEmpty() ? GenerateUniqueName() : name;
    
    auto program = MakeSharedPtr<BasicShaderProgram>();
    
    // For this basic implementation, we'll just create and return the program
    // In a real implementation, we would load the actual shader code
    
    if (program) {
        programs[progName] = program;
        return program;
    }
    
    return nullptr;
}

inline SharedPtr<ShaderProgram> ShaderManager::LoadProgramFromSource(const String& vertexSource, 
                                                                   const String& fragmentSource,
                                                                   const String& geometrySource,
                                                                   const String& name) {
    String progName = name.IsEmpty() ? GenerateUniqueName() : name;
    
    auto program = MakeSharedPtr<BasicShaderProgram>();
    
    // For this basic implementation, we'll just create and return the program
    // In a real implementation, we would compile the shader source
    
    if (program) {
        programs[progName] = program;
        return program;
    }
    
    return nullptr;
}

inline SharedPtr<ShaderProgram> ShaderManager::GetProgram(const String& name) {
    auto it = programs.find(name);
    return it != programs.end() ? it->second : nullptr;
}

inline const SharedPtr<ShaderProgram> ShaderManager::GetProgram(const String& name) const {
    auto it = programs.find(name);
    return it != programs.end() ? it->second : nullptr;
}

inline void ShaderManager::UnloadProgram(const String& name) {
    auto it = programs.find(name);
    if (it != programs.end()) {
        programs.erase(it);
    }
}

inline void ShaderManager::UnloadAllPrograms() {
    programs.clear();
    used_memory = 0;
}

inline bool ShaderManager::HasProgram(const String& name) const {
    return programs.find(name) != programs.end();
}

inline size_t ShaderManager::EstimateShaderSize(const ShaderProgram& program) const {
    // Placeholder implementation
    return 1024; // Assume 1KB per shader program for estimation
}

inline String ShaderManager::GenerateUniqueName(const String& baseName) {
    String base = baseName.IsEmpty() ? "shader" : baseName;
    return base + AsString(++name_counter);
}

// BasicShaderProgram Implementation
inline BasicShaderProgram::BasicShaderProgram() {
    // Initialize the basic shader program
}

inline bool BasicShaderProgram::AddShaderStage(const ShaderStage& stage) {
    // In basic implementation, just add to the list
    // In real implementation, would add to the actual shader compilation process
    return true;
}

inline void BasicShaderProgram::ClearShaders() {
    // Clear all shaders
    // In real implementation, would clear the actual shader stages
}

inline bool BasicShaderProgram::Link() {
    // In basic implementation, just mark as linked
    // In real implementation, would link the actual shaders
    linked = true;
    return true;
}

inline void BasicShaderProgram::Use() {
    // In basic implementation, just mark as in use
    // In real implementation, would bind the actual shader program
}

inline int BasicShaderProgram::GetUniformLocation(const String& name) {
    // In basic implementation, return a unique id
    // In real implementation, would return the actual uniform location
    static int uniform_counter = 0;
    return ++uniform_counter;
}

inline void BasicShaderProgram::SetUniform(const String& name, int value) {
    uniforms[name] = value;
    uniforms_dirty = true;
}

inline void BasicShaderProgram::SetUniform(const String& name, double value) {
    uniforms[name] = value;
    uniforms_dirty = true;
}

inline void BasicShaderProgram::SetUniform(const String& name, float value) {
    uniforms[name] = (double)value;
    uniforms_dirty = true;
}

inline void BasicShaderProgram::SetUniform(const String& name, const Point3& value) {
    uniforms[name] = value;
    uniforms_dirty = true;
}

inline void BasicShaderProgram::SetUniform(const String& name, const Vector3& value) {
    uniforms[name] = value;
    uniforms_dirty = true;
}

inline void BasicShaderProgram::SetUniform(const String& name, const Matrix4& value) {
    uniforms[name] = value;
    uniforms_dirty = true;
}

inline void BasicShaderProgram::SetUniform(const String& name, const Color& value) {
    uniforms[name] = value;
    uniforms_dirty = true;
}

inline void BasicShaderProgram::ApplyEffect(Draw& draw, const std::function<void(Draw&)>& renderFunc) {
    // In this basic implementation, we'll just call the render function
    // In a real implementation, this would apply actual shader effects
    renderFunc(draw);
}

inline Color BasicShaderProgram::GetColorFromUniforms() const {
    auto it = uniforms.find("color");
    if (it != uniforms.end()) {
        if (it->second.Is<Color>()) {
            return it->second;
        }
        if (it->second.Is<Point3>()) {
            Point3 p = it->second;
            return Color((int)p.x, (int)p.y, (int)p.z);
        }
    }
    return White(); // Default color
}

inline double BasicShaderProgram::GetScalarFromUniforms(const String& name, double defaultValue) const {
    auto it = uniforms.find(name);
    if (it != uniforms.end() && it->second.Is<double>()) {
        return it->second;
    }
    return defaultValue;
}

NAMESPACE_UPP_END

#endif