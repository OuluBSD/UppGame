#ifndef UPP_MODEL_H
#define UPP_MODEL_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/SpriteMesh.h>
#include <GameEngine/Material.h>
#include <Vector/Vector.h>
#include <HashMap/HashMap.h>

NAMESPACE_UPP

// Represents a 3D model with multiple meshes, materials, and nodes
class Model {
public:
    Model();
    explicit Model(const String& filename);
    virtual ~Model() = default;

    // Load a model from a file
    bool LoadFromFile(const String& filename);

    // Get/set model name
    void SetName(const String& name) { this->name = name; }
    const String& GetName() const { return name; }

    // Add/remove meshes
    void AddMesh(const Mesh& mesh);
    void AddMesh(const String& name, const Mesh& mesh);
    int GetMeshCount() const { return meshes.GetCount(); }
    const Mesh& GetMesh(int index) const { return meshes[index]; }
    const Mesh& GetMesh(const String& name) const;
    
    // Access all meshes
    const Vector<Mesh>& GetMeshes() const { return meshes; }
    const Vector<String>& GetMeshNames() const { return meshNames; }

    // Add/remove materials
    void AddMaterial(const Material& material);
    void AddMaterial(const String& name, const Material& material);
    int GetMaterialCount() const { return materials.GetCount(); }
    const Material& GetMaterial(int index) const { return materials[index]; }
    const Material& GetMaterial(const String& name) const;
    
    // Access all materials
    const Vector<Material>& GetMaterials() const { return materials; }
    const Vector<String>& GetMaterialNames() const { return materialNames; }

    // Skeleton and animation data (simplified for now)
    void SetHasSkeleton(bool hasSkeleton) { this->hasSkeleton = hasSkeleton; }
    bool HasSkeleton() const { return hasSkeleton; }

    // Bounding box of the entire model
    void SetBoundingBox(const Rect3& bbox) { boundingBox = bbox; }
    Rect3 GetBoundingBox() const { return boundingBox; }

    // Calculate bounding box based on all meshes
    Rect3 CalculateBoundingBox() const;

    // Memory size estimation
    int64 GetMemorySize() const;

    // Model properties
    Size GetModelSize() const { return modelSize; }
    void SetModelSize(const Size& size) { modelSize = size; }

    // Set a root transform for the entire model
    void SetTransform(const Matrix4& transform) { this->transform = transform; }
    const Matrix4& GetTransform() const { return transform; }

    // Get/set the file path this model was loaded from
    const String& GetFilePath() const { return filePath; }
    void SetFilePath(const String& path) { filePath = path; }

protected:
    String name;
    String filePath;
    
    // Meshes and materials
    Vector<Mesh> meshes;
    Vector<String> meshNames;  // Names corresponding to each mesh
    HashMap<String, int> meshNameToIndex;  // For fast lookup by name
    
    Vector<Material> materials;
    Vector<String> materialNames;  // Names corresponding to each material
    HashMap<String, int> materialNameToIndex;  // For fast lookup by name

    // Model properties
    Rect3 boundingBox;
    Size modelSize;
    Matrix4 transform = Matrix4::Identity();
    bool hasSkeleton = false;
};

END_UPP_NAMESPACE

#endif