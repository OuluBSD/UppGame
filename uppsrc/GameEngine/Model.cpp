#include "GameEngine.h"
#include "Model.h"

NAMESPACE_UPP

Model::Model() : boundingBox(Rect3(Point3(0,0,0), Point3(0,0,0))), 
                 modelSize(Size(0,0)), 
                 hasSkeleton(false) {
}

Model::Model(const String& filename) : Model() {
    LoadFromFile(filename);
}

bool Model::LoadFromFile(const String& filename) {
    filePath = filename;
    
    // In a real implementation, this would load the model from the specified file format
    // For now, we'll just set a placeholder name and return true
    name = GetFileTitle(filename);
    
    return true; // Placeholder - implement actual file loading
}

void Model::AddMesh(const Mesh& mesh) {
    meshes.Add(mesh);
    meshNames.Add(AsString(meshes.GetCount() - 1)); // Use index as name by default
}

void Model::AddMesh(const String& name, const Mesh& mesh) {
    meshes.Add(mesh);
    meshNames.Add(name);
    meshNameToIndex.GetAdd(name) = meshes.GetCount() - 1;
}

const Mesh& Model::GetMesh(const String& name) const {
    const int* index = meshNameToIndex.Get(name);
    if (index && *index >= 0 && *index < meshes.GetCount()) {
        return meshes[*index];
    }
    // Return first mesh if not found
    if (!meshes.IsEmpty()) {
        return meshes[0];
    }
    // Return a default mesh if no meshes exist
    static Mesh defaultMesh;
    return defaultMesh;
}

void Model::AddMaterial(const Material& material) {
    materials.Add(material);
    materialNames.Add(AsString(materials.GetCount() - 1)); // Use index as name by default
}

void Model::AddMaterial(const String& name, const Material& material) {
    materials.Add(material);
    materialNames.Add(name);
    materialNameToIndex.GetAdd(name) = materials.GetCount() - 1;
}

const Material& Model::GetMaterial(const String& name) const {
    const int* index = materialNameToIndex.Get(name);
    if (index && *index >= 0 && *index < materials.GetCount()) {
        return materials[*index];
    }
    // Return first material if not found
    if (!materials.IsEmpty()) {
        return materials[0];
    }
    // Return a default material if no materials exist
    static Material defaultMaterial;
    return defaultMaterial;
}

Rect3 Model::CalculateBoundingBox() const {
    if (meshes.IsEmpty()) {
        return Rect3(Point3(0, 0, 0), Point3(0, 0, 0));
    }

    double minX = 0, minY = 0, minZ = 0;
    double maxX = 0, maxY = 0, maxZ = 0;

    bool first = true;
    for (const auto& mesh : meshes) {
        const Vector<Vertex>& vertices = mesh.GetVertices();
        if (vertices.IsEmpty()) continue;

        if (first) {
            minX = maxX = vertices[0].position.x;
            minY = maxY = vertices[0].position.y;
            minZ = maxZ = vertices[0].position.z;
            first = false;
        }

        for (const auto& vertex : vertices) {
            minX = min(minX, vertex.position.x);
            minY = min(minY, vertex.position.y);
            minZ = min(minZ, vertex.position.z);
            
            maxX = max(maxX, vertex.position.x);
            maxY = max(maxY, vertex.position.y);
            maxZ = max(maxZ, vertex.position.z);
        }
    }

    return Rect3(Point3(minX, minY, minZ), Point3(maxX, maxY, maxZ));
}

int64 Model::GetMemorySize() const {
    int64 size = 0;
    
    // Calculate size of meshes
    for (const auto& mesh : meshes) {
        size += sizeof(Vertex) * mesh.GetVertices().GetCount();
        size += sizeof(int) * mesh.GetIndices().GetCount();
    }
    
    // Calculate size of materials
    size += sizeof(Material) * materials.GetCount();
    
    // Add size of name strings and other data
    size += sizeof(String) * (meshNames.GetCount() + materialNames.GetCount());
    
    return size;
}

END_UPP_NAMESPACE