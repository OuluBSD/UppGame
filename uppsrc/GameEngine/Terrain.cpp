#include "Terrain.h"
#include <plugin/png/Png.h>  // For loading heightmap images

NAMESPACE_UPP

Terrain::Terrain() {
}

bool Terrain::Initialize(int width, int depth, double scale) {
    if (width <= 0 || depth <= 0) return false;
    
    this->width = width;
    this->depth = depth;
    this->scale = scale;
    
    // Initialize height data to 0
    heightData.SetCount(width * depth);
    for (int i = 0; i < heightData.GetCount(); i++) {
        heightData[i] = 0.0;
    }
    
    // Generate the initial mesh
    UpdateMesh();
    GenerateNormals();
    
    return true;
}

bool Terrain::LoadHeightmap(const String& heightmapPath) {
    // Load the heightmap image
    Image heightmap = LoadImageFile(heightmapPath);
    if (heightmap.IsEmpty()) {
        return false;
    }
    
    int imgWidth = heightmap.GetWidth();
    int imgHeight = heightmap.GetHeight();
    
    // If we haven't initialized the terrain yet, initialize it to match the image
    if (width == 0 || depth == 0) {
        Initialize(imgWidth, imgHeight, 1.0);
    }
    
    // If the image size doesn't match our terrain, we'll resample
    heightData.SetCount(width * depth);
    
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            // Calculate corresponding position in the heightmap
            int srcX = (x * imgWidth) / width;
            int srcZ = (z * imgHeight) / depth;
            
            // Get the height from the grayscale value of the pixel
            RGBA pixel = heightmap[srcZ][srcX];
            double height = (pixel.r + pixel.g + pixel.b) / 3.0;  // Average RGB as grayscale
            height = (height / 255.0) * 2.0 - 1.0;  // Normalize to [-1, 1]
            
            // Store the height (scaled)
            heightData[GetIndex(x, z)] = height * scale;
        }
    }
    
    // Update the mesh with new height data
    UpdateMesh();
    GenerateNormals();
    
    return true;
}

bool Terrain::LoadFromData(const Vector<float>& heightData, int width, int depth) {
    if (heightData.GetCount() != width * depth) {
        return false;
    }
    
    this->width = width;
    this->depth = depth;
    this->heightData = heightData;
    
    UpdateMesh();
    GenerateNormals();
    
    return true;
}

bool Terrain::GenerateFromFunction(std::function<double(double, double)> heightFunction) {
    if (width <= 0 || depth <= 0) return false;
    
    heightData.SetCount(width * depth);
    
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            double normalizedX = (double)x / (width - 1) * 2.0 - 1.0;  // [-1, 1]
            double normalizedZ = (double)z / (depth - 1) * 2.0 - 1.0;  // [-1, 1]
            
            heightData[GetIndex(x, z)] = heightFunction(normalizedX, normalizedZ) * scale;
        }
    }
    
    UpdateMesh();
    GenerateNormals();
    
    return true;
}

double Terrain::GetHeightAt(double x, double z) const {
    // Convert world coordinates to terrain coordinates
    int terrainX = (int)(x / scale);
    int terrainZ = (int)(z / scale);
    
    // Check bounds
    if (terrainX < 0 || terrainX >= width - 1 || terrainZ < 0 || terrainZ >= depth - 1) {
        return 0.0;
    }
    
    // Interpolate height between four surrounding points
    return InterpolateHeight(x / scale, z / scale);
}

void Terrain::UpdateMesh() {
    if (width <= 0 || depth <= 0) return;
    
    mesh = Mesh();  // Clear existing mesh
    
    // Create vertices for the terrain
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            double height = heightData[GetIndex(x, z)];
            Point3 pos(x * scale, height, z * scale);
            
            // For now, use a simple normal (will be updated in GenerateNormals)
            Vector3 normal(0, 1, 0);
            
            // Simple UV coordinates based on position
            Point2 uv((double)x / width, (double)z / depth);
            
            Vertex vertex(pos, normal, uv);
            mesh.AddVertex(vertex);
        }
    }
    
    // Create indices for the terrain (triangle strips)
    for (int z = 0; z < depth - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            // First triangle
            int i1 = GetIndex(x, z);
            int i2 = GetIndex(x + 1, z);
            int i3 = GetIndex(x, z + 1);
            
            int baseIndex = mesh.GetVertices().GetCount();
            mesh.AddIndex(i1);
            mesh.AddIndex(i2);
            mesh.AddIndex(i3);
            
            // Second triangle
            int i4 = GetIndex(x + 1, z);
            int i5 = GetIndex(x + 1, z + 1);
            int i6 = GetIndex(x, z + 1);
            
            mesh.AddIndex(i4);
            mesh.AddIndex(i5);
            mesh.AddIndex(i6);
        }
    }
}

void Terrain::GenerateNormals() {
    if (width <= 0 || depth <= 0) return;
    
    normals.SetCount(width * depth);
    
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            // Calculate normal based on neighboring heights
            double hL = (x > 0) ? heightData[GetIndex(x - 1, z)] : heightData[GetIndex(x, z)];
            double hR = (x < width - 1) ? heightData[GetIndex(x + 1, z)] : heightData[GetIndex(x, z)];
            double hD = (z > 0) ? heightData[GetIndex(x, z - 1)] : heightData[GetIndex(x, z)];
            double hU = (z < depth - 1) ? heightData[GetIndex(x, z + 1)] : heightData[GetIndex(x, z)];
            
            Vector3 normal(
                hL - hR,  // Change in height along X
                2.0,      // Scale factor (2.0 is an arbitrary scale)
                hD - hU   // Change in height along Z
            );
            
            normals[GetIndex(x, z)] = normal.Normalize();
        }
    }
    
    // Update vertex normals in the mesh
    auto& vertices = const_cast<Vector<Vertex>&>(mesh.GetVertices());
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            int vertexIndex = GetIndex(x, z);
            if (vertexIndex < vertices.GetCount()) {
                vertices[vertexIndex].normal = normals[vertexIndex];
            }
        }
    }
}

void Terrain::GenerateTexCoords() {
    // Update texture coordinates in the mesh
    auto& vertices = const_cast<Vector<Vertex>&>(mesh.GetVertices());
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            int vertexIndex = GetIndex(x, z);
            if (vertexIndex < vertices.GetCount()) {
                // Simple UV mapping based on position
                vertices[vertexIndex].texCoord = Point2((double)x / width, (double)z / depth);
            }
        }
    }
}

void Terrain::Render(Draw& draw, const Matrix4& viewProjection, const Rect& viewport, 
                    const Color& color) {
    MeshRenderer renderer;
    renderer.Render(draw, mesh, viewProjection, viewport, color);
}

Rect3 Terrain::GetBounds() const {
    if (heightData.IsEmpty()) {
        return Rect3(Point3(0, 0, 0), Point3(0, 0, 0));
    }
    
    double minX = 0, minY = heightData[0], minZ = 0;
    double maxX = width * scale, maxY = heightData[0], maxZ = depth * scale;
    
    for (int i = 0; i < heightData.GetCount(); i++) {
        double height = heightData[i];
        minY = min(minY, height);
        maxY = max(maxY, height);
    }
    
    return Rect3(Point3(minX, minY, minZ), Point3(maxX, maxY, maxZ));
}

Vector3 Terrain::GetNormalAt(double x, double z) const {
    // Convert world coordinates to terrain coordinates
    int terrainX = (int)(x / scale);
    int terrainZ = (int)(z / scale);
    
    // Check bounds
    if (terrainX < 0 || terrainX >= width || terrainZ < 0 || terrainZ >= depth) {
        return Vector3(0, 1, 0);  // Default up normal
    }
    
    // Interpolate normal between four surrounding points
    return InterpolateNormal(x / scale, z / scale);
}

double Terrain::InterpolateHeight(double x, double z) const {
    // Get integer and fractional parts
    int x0 = (int)floor(x);
    int z0 = (int)floor(z);
    
    double fx = x - x0;
    double fz = z - z0;
    
    // Get the four surrounding height values
    double h00 = (x0 >= 0 && z0 >= 0 && x0 < width && z0 < depth) ? 
                 heightData[GetIndex(x0, z0)] : 0.0;
    double h10 = (x0 + 1 < width && z0 >= 0 && z0 < depth) ? 
                 heightData[GetIndex(x0 + 1, z0)] : 0.0;
    double h01 = (x0 >= 0 && x0 < width && z0 + 1 < depth) ? 
                 heightData[GetIndex(x0, z0 + 1)] : 0.0;
    double h11 = (x0 + 1 < width && z0 + 1 < depth) ? 
                 heightData[GetIndex(x0 + 1, z0 + 1)] : 0.0;
    
    // Bilinear interpolation
    double h0 = h00 * (1 - fx) + h10 * fx;
    double h1 = h01 * (1 - fx) + h11 * fx;
    
    return h0 * (1 - fz) + h1 * fz;
}

Vector3 Terrain::InterpolateNormal(double x, double z) const {
    // Similar to height interpolation but for normals
    int x0 = (int)floor(x);
    int z0 = (int)floor(z);
    
    double fx = x - x0;
    double fz = z - z0;
    
    // Get the four surrounding normal values
    Vector3 n00 = (x0 >= 0 && z0 >= 0 && x0 < width && z0 < depth) ? 
                  normals[GetIndex(x0, z0)] : Vector3(0, 1, 0);
    Vector3 n10 = (x0 + 1 < width && z0 >= 0 && z0 < depth) ? 
                  normals[GetIndex(x0 + 1, z0)] : Vector3(0, 1, 0);
    Vector3 n01 = (x0 >= 0 && x0 < width && z0 + 1 < depth) ? 
                  normals[GetIndex(x0, z0 + 1)] : Vector3(0, 1, 0);
    Vector3 n11 = (x0 + 1 < width && z0 + 1 < depth) ? 
                  normals[GetIndex(x0 + 1, z0 + 1)] : Vector3(0, 1, 0);
    
    // Interpolate along x
    Vector3 n0 = n00 * (1 - fx) + n10 * fx;
    Vector3 n1 = n01 * (1 - fx) + n11 * fx;
    
    // Interpolate along z and normalize
    Vector3 result = n0 * (1 - fz) + n1 * fz;
    return result.Normalize();
}

AdvancedTerrain::AdvancedTerrain() {
}

void AdvancedTerrain::AddLayer(const TerrainLayer& layer) {
    layers.Add(layer);
}

void AdvancedTerrain::RemoveLayer(int index) {
    if (index >= 0 && index < layers.GetCount()) {
        layers.Remove(index);
    }
}

void AdvancedTerrain::GenerateTextureBlendMap() {
    if (width <= 0 || depth <= 0) return;
    
    // Create a blend map image with multiple channels
    ImageBuffer blendBuf(width, depth);
    
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            double height = heightData[GetIndex(x, z)];
            Vector3 normal = normals[GetIndex(x, z)];
            
            // Calculate slope (angle from horizontal)
            double slope = acos(clamp(abs(normal.y), 0.0, 1.0)) * 180.0 / M_PI;
            
            // Determine layer weights based on height and slope
            RGBA pixel(0, 0, 0, 0);
            if (layers.GetCount() > 0) {
                // Use the first layer as default
                if (height >= layers[0].minHeight && height <= layers[0].maxHeight &&
                    slope >= layers[0].minSlope && slope <= layers[0].maxSlope) {
                    pixel.r = 255;  // Use red channel for first layer
                }
            }
            if (layers.GetCount() > 1) {
                // Use the second layer
                if (height >= layers[1].minHeight && height <= layers[1].maxHeight &&
                    slope >= layers[1].minSlope && slope <= layers[1].maxSlope) {
                    pixel.g = 255;  // Use green channel for second layer
                }
            }
            if (layers.GetCount() > 2) {
                // Use the third layer
                if (height >= layers[2].minHeight && height <= layers[2].maxHeight &&
                    slope >= layers[2].minSlope && slope <= layers[2].maxSlope) {
                    pixel.b = 255;  // Use blue channel for third layer
                }
            }
            if (layers.GetCount() > 3) {
                // Use the fourth layer
                if (height >= layers[3].minHeight && height <= layers[3].maxHeight &&
                    slope >= layers[3].minSlope && slope <= layers[3].maxSlope) {
                    pixel.a = 255;  // Use alpha channel for fourth layer
                }
            }
            
            blendBuf[z][x] = pixel;
        }
    }
    
    blendMap = Image(blendBuf);
}

void AdvancedTerrain::AddVegetation(const String& vegetationType, 
                                   std::function<bool(double height, double slope, const Vector3& normal)> placementFunc) {
    vegetationTypes.Add(vegetationType);
    vegetationPlacementFuncs.Add(placementFunc);
}

void AdvancedTerrain::Render(Draw& draw, const Matrix4& viewProjection, const Rect& viewport) {
    // For now, render like basic terrain
    // In a full implementation, we would use the texture blend map
    // and render with multiple layered textures
    MeshRenderer renderer;
    renderer.Render(draw, mesh, viewProjection, viewport, LightGreen());  // Use a default color
}

END_UPP_NAMESPACE