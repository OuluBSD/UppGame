#ifndef UPP_TERRAIN_H
#define UPP_TERRAIN_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/SpriteMesh.h>
#include <GameEngine/Rendering.h>

NAMESPACE_UPP

// Heightmap-based terrain for 3D environments
class Terrain {
public:
    Terrain();
    virtual ~Terrain() = default;

    // Initialize terrain with dimensions and heightmap
    bool Initialize(int width, int depth, double scale = 1.0);
    bool LoadHeightmap(const String& heightmapPath);
    bool LoadFromData(const Vector<float>& heightData, int width, int depth);

    // Generate terrain from a procedural function
    bool GenerateFromFunction(std::function<double(double, double)> heightFunction);

    // Get terrain properties
    int GetWidth() const { return width; }
    int GetDepth() const { return depth; }
    double GetScale() const { return scale; }

    // Get height at a specific position (interpolated)
    double GetHeightAt(double x, double z) const;

    // Get the mesh for rendering
    const Mesh& GetMesh() const { return mesh; }
    Mesh& GetMesh() { return mesh; }

    // Update the terrain mesh (useful for dynamic terrain)
    void UpdateMesh();

    // Generate normals for proper lighting
    void GenerateNormals();

    // Generate texture coordinates
    void GenerateTexCoords();

    // Rendering
    void Render(Draw& draw, const Matrix4& viewProjection, const Rect& viewport, 
                const Color& color = White());

    // LOD (Level of Detail) management
    void SetLODLevel(int level) { lodLevel = level; }
    int GetLODLevel() const { return lodLevel; }
    void UpdateLOD();

    // Get terrain bounds
    Rect3 GetBounds() const;

    // Get normal at a specific position (interpolated)
    Vector3 GetNormalAt(double x, double z) const;

private:
    // Terrain properties
    int width = 0;
    int depth = 0;
    double scale = 1.0;  // Scale factor for terrain size and height
    Vector<float> heightData;
    Vector<Vector3> normals;  // Precomputed normals for each vertex
    
    // Rendering
    Mesh mesh;
    int lodLevel = 0;  // Level of detail (0 = full detail)
    
    // Helper methods
    double InterpolateHeight(double x, double z) const;
    Vector3 InterpolateNormal(double x, double z) const;
    int GetIndex(int x, int z) const { return z * width + x; }
};

// Terrain layer for texturing (splatting)
struct TerrainLayer {
    Image texture;
    double minHeight = 0.0;
    double maxHeight = 1.0;
    double minSlope = 0.0;  // Minimum slope for this layer
    double maxSlope = 90.0; // Maximum slope for this layer
    String name;
};

// Advanced terrain with multiple layers and features
class AdvancedTerrain : public Terrain {
public:
    AdvancedTerrain();
    virtual ~AdvancedTerrain() = default;

    // Add/remove terrain layers for texturing
    void AddLayer(const TerrainLayer& layer);
    void RemoveLayer(int index);
    int GetLayerCount() const { return layers.GetCount(); }
    const TerrainLayer& GetLayer(int index) const { return layers[index]; }

    // Blend terrain textures based on height and slope
    void GenerateTextureBlendMap();

    // Add vegetation (trees, grass) based on terrain properties
    void AddVegetation(const String& vegetationType, 
                      std::function<bool(double height, double slope, const Vector3& normal)> placementFunc);

    // Render with texturing
    void Render(Draw& draw, const Matrix4& viewProjection, const Rect& viewport);

private:
    Vector<TerrainLayer> layers;
    Image blendMap;  // Texture blend information
    Vector<String> vegetationTypes;  // Types of vegetation to place
    Vector<std::function<bool(double, double, const Vector3&)>> vegetationPlacementFuncs;
};

END_UPP_NAMESPACE

#endif