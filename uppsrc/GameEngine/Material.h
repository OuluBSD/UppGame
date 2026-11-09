#ifndef UPP_MATERIAL_H
#define UPP_MATERIAL_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP_BEGIN

// Material property types
enum class MaterialPropertyType {
    COLOR,
    TEXTURE,
    FLOAT,
    VECTOR2,
    VECTOR3,
    VECTOR4,
    MATRIX4
};

// Material property variant
class MaterialProperty {
public:
    MaterialProperty() : type(MaterialPropertyType::COLOR), value(White()) {}
    
    // Constructors for different types
    MaterialProperty(Color c) : type(MaterialPropertyType::COLOR), value(c) {}
    MaterialProperty(double f) : type(MaterialPropertyType::FLOAT), value(f) {}
    MaterialProperty(const Point2& v) : type(MaterialPropertyType::VECTOR2), value(v) {}
    MaterialProperty(const Point3& v) : type(MaterialPropertyType::VECTOR3), value(v) {}
    MaterialProperty(const Point4& v) : type(MaterialPropertyType::VECTOR4), value(v) {}
    MaterialProperty(const Matrix4& m) : type(MaterialPropertyType::MATRIX4), value(m) {}
    MaterialProperty(const Image& img) : type(MaterialPropertyType::TEXTURE), value(img) {}
    
    // Getters
    Color GetColor() const { 
        if (type == MaterialPropertyType::COLOR) {
            return Get<Color>(value);
        }
        return White(); 
    }
    
    double GetFloat() const { 
        if (type == MaterialPropertyType::FLOAT) {
            return Get<double>(value);
        }
        return 1.0; 
    }
    
    Point2 GetVector2() const { 
        if (type == MaterialPropertyType::VECTOR2) {
            return Get<Point2>(value);
        }
        return Point2(0, 0); 
    }
    
    Point3 GetVector3() const { 
        if (type == MaterialPropertyType::VECTOR3) {
            return Get<Point3>(value);
        }
        return Point3(0, 0, 0); 
    }
    
    Point4 GetVector4() const { 
        if (type == MaterialPropertyType::VECTOR4) {
            return Get<Point4>(value);
        }
        return Point4(0, 0, 0, 0); 
    }
    
    Matrix4 GetMatrix4() const { 
        if (type == MaterialPropertyType::MATRIX4) {
            return Get<Matrix4>(value);
        }
        return Matrix4::Identity(); 
    }
    
    Image GetTexture() const { 
        if (type == MaterialPropertyType::TEXTURE) {
            return Get<Image>(value);
        }
        return Image(); 
    }
    
    // Setters
    void Set(Color c) { type = MaterialPropertyType::COLOR; value = c; }
    void Set(double f) { type = MaterialPropertyType::FLOAT; value = f; }
    void Set(const Point2& v) { type = MaterialPropertyType::VECTOR2; value = v; }
    void Set(const Point3& v) { type = MaterialPropertyType::VECTOR3; value = v; }
    void Set(const Point4& v) { type = MaterialPropertyType::VECTOR4; value = v; }
    void Set(const Matrix4& m) { type = MaterialPropertyType::MATRIX4; value = m; }
    void Set(const Image& img) { type = MaterialPropertyType::TEXTURE; value = img; }
    
    // Type checker
    MaterialPropertyType GetType() const { return type; }
    
    // Equality operator
    bool operator==(const MaterialProperty& other) const {
        if (type != other.type) return false;
        
        switch (type) {
            case MaterialPropertyType::COLOR: 
                return Get<Color>(value) == Get<Color>(other.value);
            case MaterialPropertyType::FLOAT: 
                return Get<double>(value) == Get<double>(other.value);
            case MaterialPropertyType::VECTOR2: 
                return Get<Point2>(value) == Get<Point2>(other.value);
            case MaterialPropertyType::VECTOR3: 
                return Get<Point3>(value) == Get<Point3>(other.value);
            case MaterialPropertyType::VECTOR4: 
                return Get<Point4>(value) == Get<Point4>(other.value);
            case MaterialPropertyType::MATRIX4: 
                return Get<Matrix4>(value) == Get<Matrix4>(other.value);
            case MaterialPropertyType::TEXTURE: 
                return Get<Image>(value) == Get<Image>(other.value);
            default: 
                return false;
        }
    }

private:
    MaterialPropertyType type;
    // Use U++'s Value type to hold different types
    Value value;
};

// Basic material class
class Material {
public:
    Material();
    virtual ~Material() = default;
    
    // Set a property by name
    void SetProperty(const String& name, const MaterialProperty& value);
    void SetProperty(const String& name, Color value);
    void SetProperty(const String& name, double value);
    void SetProperty(const String& name, const Point2& value);
    void SetProperty(const String& name, const Point3& value);
    void SetProperty(const String& name, const Point4& value);
    void SetProperty(const String& name, const Matrix4& value);
    void SetProperty(const String& name, const Image& value);
    
    // Get a property by name
    MaterialProperty GetProperty(const String& name) const;
    Color GetColorProperty(const String& name) const;
    double GetFloatProperty(const String& name) const;
    Point2 GetVector2Property(const String& name) const;
    Point3 GetVector3Property(const String& name) const;
    Point4 GetVector4Property(const String& name) const;
    Matrix4 GetMatrix4Property(const String& name) const;
    Image GetTextureProperty(const String& name) const;
    
    // Check if property exists
    bool HasProperty(const String& name) const;
    
    // Remove a property
    void RemoveProperty(const String& name);
    
    // Get all property names
    Vector<String> GetPropertyNames() const;
    
    // Set the shader program for this material
    void SetShader(SharedPtr<ShaderProgram> shader) { this->shader = shader; }
    SharedPtr<ShaderProgram> GetShader() const { return shader; }
    
    // Set texture by name (convenience method)
    void SetTexture(const String& name, const Image& texture);
    
    // Get texture by name (convenience method)
    Image GetTexture(const String& name) const;
    
    // Basic material properties (commonly used)
    void SetAlbedo(Color color) { SetProperty("_Albedo", color); }
    void SetEmission(Color color) { SetProperty("_Emission", color); }
    void SetNormalMap(const Image& normal) { SetTexture("_NormalMap", normal); }
    void SetRoughness(double roughness) { SetProperty("_Roughness", roughness); }
    void SetMetallic(double metallic) { SetProperty("_Metallic", metallic); }
    
    Color GetAlbedo() const { return GetColorProperty("_Albedo"); }
    Color GetEmission() const { return GetColorProperty("_Emission"); }
    Image GetNormalMap() const { return GetTexture("_NormalMap"); }
    double GetRoughness() const { return GetFloatProperty("_Roughness"); }
    double GetMetallic() const { return GetFloatProperty("_Metallic"); }
    
    // Apply this material to a draw context
    virtual void Apply(Draw& draw);
    
    // Get material ID
    int GetId() const { return id; }
    
private:
    std::map<String, MaterialProperty> properties;
    SharedPtr<ShaderProgram> shader;
    int id;
    static int next_id;
};

// Material manager for loading, caching and managing materials
class MaterialManager {
public:
    MaterialManager();
    ~MaterialManager();
    
    // Create a new material
    SharedPtr<Material> CreateMaterial(const String& name = String());
    
    // Load material from file (TBD format - could be JSON, etc.)
    SharedPtr<Material> LoadMaterialFromFile(const String& filepath, const String& name = String());
    
    // Get material by name
    SharedPtr<Material> GetMaterial(const String& name);
    const SharedPtr<Material> GetMaterial(const String& name) const;
    
    // Get material by ID
    SharedPtr<Material> GetMaterialById(int id);
    
    // Remove material by name
    void RemoveMaterial(const String& name);
    
    // Remove all materials
    void RemoveAllMaterials();
    
    // Get all material names
    Vector<String> GetMaterialNames() const;
    
    // Check if material exists
    bool HasMaterial(const String& name) const;
    
    // Get/set memory budget for materials (in bytes)
    void SetMemoryBudget(uint64 budget) { memory_budget = budget; }
    uint64 GetMemoryBudget() const { return memory_budget; }
    uint64 GetUsedMemory() const { return used_memory; }
    
private:
    std::map<String, SharedPtr<Material>> materials;
    std::map<int, SharedPtr<Material>> materials_by_id;
    uint64 memory_budget = 50 * 1024 * 1024; // 50MB default
    uint64 used_memory = 0;
    
    // Helper to estimate material memory usage
    size_t EstimateMaterialSize(const Material& material) const;
    
    // Generate unique name if not provided
    String GenerateUniqueName(const String& baseName = "material");
    int name_counter = 0;
};

// Predefined basic materials
class BasicMaterials {
public:
    // Create a basic unlit material
    static SharedPtr<Material> CreateUnlitMaterial(Color color = White());
    
    // Create a basic lit material
    static SharedPtr<Material> CreateLitMaterial(Color albedo = White(), 
                                               double roughness = 0.5, 
                                               double metallic = 0.0);
    
    // Create a basic textured material
    static SharedPtr<Material> CreateTexturedMaterial(const Image& texture);
    
    // Create a basic sprite material
    static SharedPtr<Material> CreateSpriteMaterial();
};

// Implementation
inline int Material::next_id = 0;

inline Material::Material() : id(next_id++) {
    // Set default properties
    SetAlbedo(White());
    SetRoughness(0.5);
    SetMetallic(0.0);
}

inline void Material::SetProperty(const String& name, const MaterialProperty& value) {
    properties[name] = value;
}

inline void Material::SetProperty(const String& name, Color value) {
    properties[name] = MaterialProperty(value);
}

inline void Material::SetProperty(const String& name, double value) {
    properties[name] = MaterialProperty(value);
}

inline void Material::SetProperty(const String& name, const Point2& value) {
    properties[name] = MaterialProperty(value);
}

inline void Material::SetProperty(const String& name, const Point3& value) {
    properties[name] = MaterialProperty(value);
}

inline void Material::SetProperty(const String& name, const Point4& value) {
    properties[name] = MaterialProperty(value);
}

inline void Material::SetProperty(const String& name, const Matrix4& value) {
    properties[name] = MaterialProperty(value);
}

inline void Material::SetProperty(const String& name, const Image& value) {
    properties[name] = MaterialProperty(value);
}

inline MaterialProperty Material::GetProperty(const String& name) const {
    auto it = properties.find(name);
    return it != properties.end() ? it->second : MaterialProperty();
}

inline Color Material::GetColorProperty(const String& name) const {
    auto prop = GetProperty(name);
    return prop.GetType() == MaterialPropertyType::COLOR ? prop.GetColor() : White();
}

inline double Material::GetFloatProperty(const String& name) const {
    auto prop = GetProperty(name);
    return prop.GetType() == MaterialPropertyType::FLOAT ? prop.GetFloat() : 1.0;
}

inline Point2 Material::GetVector2Property(const String& name) const {
    auto prop = GetProperty(name);
    return prop.GetType() == MaterialPropertyType::VECTOR2 ? prop.GetVector2() : Point2(0, 0);
}

inline Point3 Material::GetVector3Property(const String& name) const {
    auto prop = GetProperty(name);
    return prop.GetType() == MaterialPropertyType::VECTOR3 ? prop.GetVector3() : Point3(0, 0, 0);
}

inline Point4 Material::GetVector4Property(const String& name) const {
    auto prop = GetProperty(name);
    return prop.GetType() == MaterialPropertyType::VECTOR4 ? prop.GetVector4() : Point4(0, 0, 0, 0);
}

inline Matrix4 Material::GetMatrix4Property(const String& name) const {
    auto prop = GetProperty(name);
    return prop.GetType() == MaterialPropertyType::MATRIX4 ? prop.GetMatrix4() : Matrix4::Identity();
}

inline Image Material::GetTextureProperty(const String& name) const {
    auto prop = GetProperty(name);
    return prop.GetType() == MaterialPropertyType::TEXTURE ? prop.GetTexture() : Image();
}

inline bool Material::HasProperty(const String& name) const {
    return properties.find(name) != properties.end();
}

inline void Material::RemoveProperty(const String& name) {
    properties.erase(name);
}

inline Vector<String> Material::GetPropertyNames() const {
    Vector<String> names;
    for (const auto& pair : properties) {
        names.Add(pair.first);
    }
    return names;
}

inline void Material::SetTexture(const String& name, const Image& texture) {
    SetProperty(name, texture);
}

inline Image Material::GetTexture(const String& name) const {
    auto prop = GetProperty(name);
    return prop.GetType() == MaterialPropertyType::TEXTURE ? prop.GetTexture() : Image();
}

inline void Material::Apply(Draw& draw) {
    // In a basic implementation, just apply the albedo color
    // In a full implementation, this would bind textures and shader properties
    Color albedo = GetAlbedo();
    // Could use this color for drawing operations in a basic context
    // For now, just a placeholder implementation
}

inline MaterialManager::MaterialManager() {
    // Initialize the material manager
}

inline MaterialManager::~MaterialManager() {
    RemoveAllMaterials();
}

inline SharedPtr<Material> MaterialManager::CreateMaterial(const String& name) {
    String matName = name.IsEmpty() ? GenerateUniqueName() : name;
    
    auto material = MakeSharedPtr<Material>();
    
    if (material) {
        materials[matName] = material;
        materials_by_id[material->GetId()] = material;
        return material;
    }
    
    return nullptr;
}

inline SharedPtr<Material> MaterialManager::LoadMaterialFromFile(const String& filepath, const String& name) {
    // In a real implementation, this would load material properties from a file
    // For now, just create a default material
    String matName = name.IsEmpty() ? GenerateUniqueName() : name;
    return CreateMaterial(matName);
}

inline SharedPtr<Material> MaterialManager::GetMaterial(const String& name) {
    auto it = materials.find(name);
    return it != materials.end() ? it->second : nullptr;
}

inline const SharedPtr<Material> MaterialManager::GetMaterial(const String& name) const {
    auto it = materials.find(name);
    return it != materials.end() ? it->second : nullptr;
}

inline SharedPtr<Material> MaterialManager::GetMaterialById(int id) {
    auto it = materials_by_id.find(id);
    return it != materials_by_id.end() ? it->second : nullptr;
}

inline void MaterialManager::RemoveMaterial(const String& name) {
    auto it = materials.find(name);
    if (it != materials.end()) {
        int id = it->second->GetId();
        materials.erase(it);
        materials_by_id.erase(id);
    }
}

inline void MaterialManager::RemoveAllMaterials() {
    materials.clear();
    materials_by_id.clear();
    used_memory = 0;
}

inline Vector<String> MaterialManager::GetMaterialNames() const {
    Vector<String> names;
    for (const auto& pair : materials) {
        names.Add(pair.first);
    }
    return names;
}

inline bool MaterialManager::HasMaterial(const String& name) const {
    return materials.find(name) != materials.end();
}

inline size_t MaterialManager::EstimateMaterialSize(const Material& material) const {
    // Rough estimate based on number of properties
    return material.GetPropertyNames().GetCount() * 128; // 128 bytes per property estimate
}

inline String MaterialManager::GenerateUniqueName(const String& baseName) {
    String base = baseName.IsEmpty() ? "material" : baseName;
    return base + AsString(++name_counter);
}

// Basic materials implementation
inline SharedPtr<Material> BasicMaterials::CreateUnlitMaterial(Color color) {
    auto material = MakeSharedPtr<Material>();
    material->SetAlbedo(color);
    material->SetProperty("_Unlit", 1.0); // Mark as unlit
    return material;
}

inline SharedPtr<Material> BasicMaterials::CreateLitMaterial(Color albedo, double roughness, double metallic) {
    auto material = MakeSharedPtr<Material>();
    material->SetAlbedo(albedo);
    material->SetRoughness(roughness);
    material->SetMetallic(metallic);
    material->SetProperty("_Lit", 1.0); // Mark as lit
    return material;
}

inline SharedPtr<Material> BasicMaterials::CreateTexturedMaterial(const Image& texture) {
    auto material = MakeSharedPtr<Material>();
    material->SetTexture("_MainTex", texture);
    material->SetAlbedo(White()); // Default albedo for textured materials
    return material;
}

inline SharedPtr<Material> BasicMaterials::CreateSpriteMaterial() {
    auto material = MakeSharedPtr<Material>();
    material->SetProperty("_Sprite", 1.0); // Mark as sprite material
    material->SetAlbedo(White());
    return material;
}

NAMESPACE_UPP_END

#endif