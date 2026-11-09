#ifndef UPP_SPRITE_MESH_H
#define UPP_SPRITE_MESH_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP_BEGIN

// Vertex structure for 3D meshes
struct Vertex {
    Point3 position;
    Vector3 normal;
    Point2 texCoord;
    Color color = White();
    
    Vertex() = default;
    Vertex(const Point3& pos) : position(pos), normal(0, 1, 0), texCoord(0, 0) {}
    Vertex(const Point3& pos, const Vector3& norm, const Point2& uv) 
        : position(pos), normal(norm), texCoord(uv) {}
    Vertex(const Point3& pos, const Vector3& norm, const Point2& uv, Color c) 
        : position(pos), normal(norm), texCoord(uv), color(c) {}
};

// Basic mesh structure
class Mesh {
public:
    Mesh() = default;
    
    // Add a vertex to the mesh
    void AddVertex(const Vertex& vertex) { vertices.Add(vertex); }
    
    // Add an index to the mesh (for indexed rendering)
    void AddIndex(int index) { indices.Add(index); }
    
    // Add a triangle using 3 vertices (non-indexed)
    void AddTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
        int baseIndex = vertices.GetCount();
        vertices.Add(v1);
        vertices.Add(v2);
        vertices.Add(v3);
        
        indices.Add(baseIndex);
        indices.Add(baseIndex + 1);
        indices.Add(baseIndex + 2);
    }
    
    // Create a basic cube mesh
    static Mesh CreateCube(double size = 1.0);
    
    // Create a basic plane mesh
    static Mesh CreatePlane(double width = 1.0, double height = 1.0, int segments = 1);
    
    // Create a basic sphere mesh
    static Mesh CreateSphere(double radius = 1.0, int segments = 16);
    
    // Create a basic quad mesh (for sprites)
    static Mesh CreateQuad(double width = 1.0, double height = 1.0);
    
    // Getters
    const Vector<Vertex>& GetVertices() const { return vertices; }
    const Vector<int>& GetIndices() const { return indices; }
    
    // Setters
    void SetVertices(const Vector<Vertex>& verts) { vertices = verts; }
    void SetIndices(const Vector<int>& idx) { indices = idx; }
    
private:
    Vector<Vertex> vertices;
    Vector<int> indices;
};

// Sprite component (for 2D rendering)
struct Sprite {
    Image image;
    Point2 position = Point2(0, 0);
    Point2 size;           // Size in world units (if 0,0 then use image size)
    Point2 origin = Point2(0.5, 0.5);  // Origin point (0,0 = top-left, 0.5,0.5 = center, 1,1 = bottom-right)
    Color color = White(); // Tint color
    double rotation = 0.0; // Rotation in radians
    double layer = 0.0;    // Layer for sorting
    
    Sprite() = default;
    Sprite(const Image& img) : image(img), size(Point2(img.GetSize().cx, img.GetSize().cy)) {}
    Sprite(const String& imagePath) { 
        image = LoadImageFile(imagePath);
        size = Point2(image.GetSize().cx, image.GetSize().cy);
    }
    
    // Calculate the world-space bounds of the sprite
    Rect GetBounds() const {
        Size imgSize = image.GetSize();
        double w = size.x != 0 ? size.x : imgSize.cx;
        double h = size.y != 0 ? size.y : imgSize.cy;
        
        double offsetX = w * origin.x;
        double offsetY = h * origin.y;
        
        return Rect(Point2(position.x - offsetX, position.y - offsetY), 
                   Point2(position.x - offsetX + w, position.y - offsetY + h));
    }
};

// Mesh renderer for 3D objects
class MeshRenderer {
public:
    MeshRenderer();
    
    // Render a mesh using the basic 3D renderer
    void Render(Draw& draw, const Mesh& mesh, const Matrix4& transform, 
                const Matrix4& viewProjection, const Rect& viewport,
                const Color& color = White());
    
    // Render with texture (simplified approach)
    void Render(Draw& draw, const Mesh& mesh, const Image& texture, const Matrix4& transform,
                const Matrix4& viewProjection, const Rect& viewport);
    
    // Render wireframe
    void RenderWireframe(Draw& draw, const Mesh& mesh, const Matrix4& transform,
                         const Matrix4& viewProjection, const Rect& viewport,
                         const Color& color = White(), int lineWidth = 1);
    
    // Get bounding box of a mesh
    Rect3 GetBoundingBox(const Mesh& mesh) const;
    
private:
    // Internal method to project vertices
    Vector<Point> ProjectVertices(const Vector<Vertex>& vertices, 
                                 const Matrix4& transform,
                                 const Matrix4& viewProjection, 
                                 const Rect& viewport) const;
};

// Sprite renderer for 2D objects
class SpriteRenderer {
public:
    SpriteRenderer();
    
    // Render a single sprite
    void Render(Draw& draw, const Sprite& sprite);
    
    // Render a sprite with transformation
    void Render(Draw& draw, const Sprite& sprite, const Matrix4& transform);
    
    // Render a sprite at a specific position and size
    void Render(Draw& draw, const Image& image, const Point2& position, 
                const Point2& size = Point2(0, 0), const Color& color = White(),
                const Point2& origin = Point2(0.5, 0.5), double rotation = 0.0);
    
    // Batch render multiple sprites (sorted by layer)
    void RenderBatch(Draw& draw, const Vector<Sprite>& sprites);
    
private:
    // Helper to rotate a point around origin
    Point2 RotatePoint(const Point2& point, double angle) const;
};

// Implementation
inline Mesh Mesh::CreateCube(double size) {
    Mesh mesh;
    double half = size / 2.0;
    
    // Define vertices for a cube
    Vector<Vertex> verts = {
        // Front face
        Vertex(Point3(-half, -half,  half), Vector3(0, 0, 1), Point2(0, 1)),
        Vertex(Point3( half, -half,  half), Vector3(0, 0, 1), Point2(1, 1)),
        Vertex(Point3( half,  half,  half), Vector3(0, 0, 1), Point2(1, 0)),
        Vertex(Point3(-half,  half,  half), Vector3(0, 0, 1), Point2(0, 0)),
        
        // Back face
        Vertex(Point3(-half, -half, -half), Vector3(0, 0, -1), Point2(1, 1)),
        Vertex(Point3(-half,  half, -half), Vector3(0, 0, -1), Point2(1, 0)),
        Vertex(Point3( half,  half, -half), Vector3(0, 0, -1), Point2(0, 0)),
        Vertex(Point3( half, -half, -half), Vector3(0, 0, -1), Point2(0, 1)),
        
        // Top face
        Vertex(Point3(-half,  half, -half), Vector3(0, 1, 0), Point2(0, 1)),
        Vertex(Point3(-half,  half,  half), Vector3(0, 1, 0), Point2(0, 0)),
        Vertex(Point3( half,  half,  half), Vector3(0, 1, 0), Point2(1, 0)),
        Vertex(Point3( half,  half, -half), Vector3(0, 1, 0), Point2(1, 1)),
        
        // Bottom face
        Vertex(Point3(-half, -half, -half), Vector3(0, -1, 0), Point2(1, 1)),
        Vertex(Point3( half, -half, -half), Vector3(0, -1, 0), Point2(0, 1)),
        Vertex(Point3( half, -half,  half), Vector3(0, -1, 0), Point2(0, 0)),
        Vertex(Point3(-half, -half,  half), Vector3(0, -1, 0), Point2(1, 0)),
        
        // Right face
        Vertex(Point3( half, -half, -half), Vector3(1, 0, 0), Point2(0, 1)),
        Vertex(Point3( half,  half, -half), Vector3(1, 0, 0), Point2(0, 0)),
        Vertex(Point3( half,  half,  half), Vector3(1, 0, 0), Point2(1, 0)),
        Vertex(Point3( half, -half,  half), Vector3(1, 0, 0), Point2(1, 1)),
        
        // Left face
        Vertex(Point3(-half, -half, -half), Vector3(-1, 0, 0), Point2(1, 1)),
        Vertex(Point3(-half, -half,  half), Vector3(-1, 0, 0), Point2(0, 1)),
        Vertex(Point3(-half,  half,  half), Vector3(-1, 0, 0), Point2(0, 0)),
        Vertex(Point3(-half,  half, -half), Vector3(-1, 0, 0), Point2(1, 0))
    };
    
    // Define indices for the cube (6 faces, 2 triangles each, 3 vertices per triangle)
    Vector<int> indices = {
        // Front face
        0, 1, 2, 0, 2, 3,
        // Back face  
        4, 5, 6, 4, 6, 7,
        // Top face
        8, 9, 10, 8, 10, 11,
        // Bottom face
        12, 13, 14, 12, 14, 15,
        // Right face
        16, 17, 18, 16, 18, 19,
        // Left face
        20, 21, 22, 20, 22, 23
    };
    
    mesh.SetVertices(verts);
    mesh.SetIndices(indices);
    
    return mesh;
}

inline Mesh Mesh::CreatePlane(double width, double height, int segments) {
    Mesh mesh;
    
    // Create vertices for a plane
    double halfWidth = width / 2.0;
    double halfHeight = height / 2.0;
    
    // For simplicity, just create a single quad for now
    // In a more detailed implementation, we'd subdivide based on segments
    
    Vector<Vertex> verts = {
        Vertex(Point3(-halfWidth, -halfHeight, 0), Vector3(0, 0, 1), Point2(0, 1)),
        Vertex(Point3( halfWidth, -halfHeight, 0), Vector3(0, 0, 1), Point2(1, 1)),
        Vertex(Point3( halfWidth,  halfHeight, 0), Vector3(0, 0, 1), Point2(1, 0)),
        Vertex(Point3(-halfWidth,  halfHeight, 0), Vector3(0, 0, 1), Point2(0, 0))
    };
    
    Vector<int> indices = { 0, 1, 2, 0, 2, 3 };
    
    mesh.SetVertices(verts);
    mesh.SetIndices(indices);
    
    return mesh;
}

inline Mesh Mesh::CreateSphere(double radius, int segments) {
    Mesh mesh;
    
    Vector<Vertex> vertices;
    Vector<int> indices;
    
    // Create sphere vertices using spherical coordinates
    int stacks = segments;
    int slices = segments;
    
    for (int i = 0; i <= stacks; i++) {
        double phi = M_PI * i / stacks;  // from 0 to PI
        
        for (int j = 0; j <= slices; j++) {
            double theta = 2 * M_PI * j / slices;  // from 0 to 2*PI
            
            double x = radius * sin(phi) * cos(theta);
            double y = radius * cos(phi);
            double z = radius * sin(phi) * sin(theta);
            
            Point3 pos(x, y, z);
            Vector3 norm = pos.Normalize();
            Point2 uv((double)j / slices, (double)i / stacks);
            
            vertices.Add(Vertex(pos, norm, uv));
        }
    }
    
    // Create indices for sphere faces
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;
            
            indices.Add(first);
            indices.Add(second);
            indices.Add(first + 1);
            
            indices.Add(second);
            indices.Add(second + 1);
            indices.Add(first + 1);
        }
    }
    
    mesh.SetVertices(vertices);
    mesh.SetIndices(indices);
    
    return mesh;
}

inline Mesh Mesh::CreateQuad(double width, double height) {
    Mesh mesh;
    
    double halfWidth = width / 2.0;
    double halfHeight = height / 2.0;
    
    Vector<Vertex> verts = {
        Vertex(Point3(-halfWidth, -halfHeight, 0), Vector3(0, 0, 1), Point2(0, 1)),
        Vertex(Point3( halfWidth, -halfHeight, 0), Vector3(0, 0, 1), Point2(1, 1)),
        Vertex(Point3( halfWidth,  halfHeight, 0), Vector3(0, 0, 1), Point2(1, 0)),
        Vertex(Point3(-halfWidth,  halfHeight, 0), Vector3(0, 0, 1), Point2(0, 0))
    };
    
    Vector<int> idx = { 0, 1, 2, 0, 2, 3 };
    
    mesh.SetVertices(verts);
    mesh.SetIndices(idx);
    
    return mesh;
}

inline MeshRenderer::MeshRenderer() {
    // Initialize the mesh renderer
}

inline void MeshRenderer::Render(Draw& draw, const Mesh& mesh, const Matrix4& transform,
                                 const Matrix4& viewProjection, const Rect& viewport,
                                 const Color& color) {
    const Vector<Vertex>& vertices = mesh.GetVertices();
    const Vector<int>& indices = mesh.GetIndices();
    
    // Transform and project vertices
    Vector<Point> projected = ProjectVertices(vertices, transform, viewProjection, viewport);
    
    // Draw triangles based on indices
    for (int i = 0; i < indices.GetCount(); i += 3) {
        if (i + 2 < indices.GetCount()) {
            int i1 = indices[i];
            int i2 = indices[i + 1];
            int i3 = indices[i + 2];
            
            if (i1 < projected.GetCount() && i2 < projected.GetCount() && i3 < projected.GetCount()) {
                Vector<Point> triangle;
                triangle.Add(projected[i1]);
                triangle.Add(projected[i2]);
                triangle.Add(projected[i3]);
                
                draw.DrawPolygon(triangle, color);
            }
        }
    }
}

inline void MeshRenderer::Render(Draw& draw, const Mesh& mesh, const Image& texture, 
                                 const Matrix4& transform, const Matrix4& viewProjection, 
                                 const Rect& viewport) {
    // For this basic implementation, we'll just render the mesh with a color
    // A full implementation would handle texturing
    Render(draw, mesh, transform, viewProjection, viewport, White());
}

inline void MeshRenderer::RenderWireframe(Draw& draw, const Mesh& mesh, const Matrix4& transform,
                                          const Matrix4& viewProjection, const Rect& viewport,
                                          const Color& color, int lineWidth) {
    const Vector<Vertex>& vertices = mesh.GetVertices();
    const Vector<int>& indices = mesh.GetIndices();
    
    // Transform and project vertices
    Vector<Point> projected = ProjectVertices(vertices, transform, viewProjection, viewport);
    
    // Draw lines forming triangles based on indices
    for (int i = 0; i < indices.GetCount(); i += 3) {
        if (i + 2 < indices.GetCount()) {
            int i1 = indices[i];
            int i2 = indices[i + 1];
            int i3 = indices[i + 2];
            
            if (i1 < projected.GetCount() && i2 < projected.GetCount() && i3 < projected.GetCount()) {
                // Draw the three edges of the triangle
                draw.DrawLine(projected[i1], projected[i2], lineWidth, color);
                draw.DrawLine(projected[i2], projected[i3], lineWidth, color);
                draw.DrawLine(projected[i3], projected[i1], lineWidth, color);
            }
        }
    }
}

inline Rect3 MeshRenderer::GetBoundingBox(const Mesh& mesh) const {
    if (mesh.GetVertices().GetCount() == 0) {
        return Rect3(Point3(0, 0, 0), Point3(0, 0, 0));
    }
    
    const Vector<Vertex>& vertices = mesh.GetVertices();
    
    double minX = vertices[0].position.x;
    double minY = vertices[0].position.y;
    double minZ = vertices[0].position.z;
    double maxX = vertices[0].position.x;
    double maxY = vertices[0].position.y;
    double maxZ = vertices[0].position.z;
    
    for (const auto& vertex : vertices) {
        minX = min(minX, vertex.position.x);
        minY = min(minY, vertex.position.y);
        minZ = min(minZ, vertex.position.z);
        maxX = max(maxX, vertex.position.x);
        maxY = max(maxY, vertex.position.y);
        maxZ = max(maxZ, vertex.position.z);
    }
    
    return Rect3(Point3(minX, minY, minZ), Point3(maxX, maxY, maxZ));
}

inline Vector<Point> MeshRenderer::ProjectVertices(const Vector<Vertex>& vertices,
                                                   const Matrix4& transform,
                                                   const Matrix4& viewProjection,
                                                   const Rect& viewport) const {
    Vector<Point> projected;
    Matrix4 worldViewProj = viewProjection * transform;
    
    for (const auto& vertex : vertices) {
        Point4 homogeneous = worldViewProj.Transform(Point4(vertex.position.x, 
                                                           vertex.position.y, 
                                                           vertex.position.z, 1.0));
        if (homogeneous.w != 0) {
            Point3 ndc = Point3(homogeneous.x / homogeneous.w, 
                               homogeneous.y / homogeneous.w, 
                               homogeneous.z / homogeneous.w);
            
            int screenX = (int)((ndc.x + 1.0) * 0.5 * viewport.Width());
            int screenY = (int)((-ndc.y + 1.0) * 0.5 * viewport.Height());
            
            projected.Add(Point(screenX, screenY));
        } else {
            // If the point is at infinity, place it off-screen
            projected.Add(Point(-1, -1));
        }
    }
    
    return projected;
}

inline SpriteRenderer::SpriteRenderer() {
    // Initialize the sprite renderer
}

inline void SpriteRenderer::Render(Draw& draw, const Sprite& sprite) {
    Size imgSize = sprite.image.GetSize();
    double w = sprite.size.x != 0 ? sprite.size.x : imgSize.cx;
    double h = sprite.size.y != 0 ? sprite.size.y : imgSize.cy;
    
    double offsetX = w * sprite.origin.x;
    double offsetY = h * sprite.origin.y;
    
    // Calculate position based on origin
    Point2 renderPos(sprite.position.x - offsetX, sprite.position.y - offsetY);
    
    // For now, just draw the image at the calculated position
    // In a more advanced implementation, we'd handle rotation and color tinting
    if (sprite.image && !sprite.image.IsEmpty()) {
        // Draw using DrawImage if available, or use DrawRect as fallback
        draw.DrawImage((int)renderPos.x, (int)renderPos.y, (int)w, (int)h, sprite.image);
    } else {
        // Fallback: draw a colored rectangle
        draw.DrawRect((int)renderPos.x, (int)renderPos.y, (int)w, (int)h, sprite.color);
    }
}

inline void SpriteRenderer::Render(Draw& draw, const Sprite& sprite, const Matrix4& transform) {
    // Apply transformation to sprite rendering
    // For this basic implementation, just call the simple render
    Render(draw, sprite);
}

inline void SpriteRenderer::Render(Draw& draw, const Image& image, const Point2& position, 
                                   const Point2& size, const Color& color,
                                   const Point2& origin, double rotation) {
    Sprite sprite(image);
    sprite.position = position;
    sprite.size = size;
    sprite.origin = origin;
    sprite.color = color;
    sprite.rotation = rotation;
    
    Render(draw, sprite);
}

inline void SpriteRenderer::RenderBatch(Draw& draw, const Vector<Sprite>& sprites) {
    // Sort sprites by layer (lower layers drawn first)
    Vector<int> indices(sprites.GetCount());
    for(int i = 0; i < sprites.GetCount(); i++) 
        indices[i] = i;
        
    Sort(indices, [&](int a, int b) { 
        return sprites[a].layer < sprites[b].layer; 
    });
    
    // Render sprites in sorted order
    for (int idx : indices) {
        Render(draw, sprites[idx]);
    }
}

inline Point2 SpriteRenderer::RotatePoint(const Point2& point, double angle) const {
    double cosA = cos(angle);
    double sinA = sin(angle);
    
    return Point2(
        point.x * cosA - point.y * sinA,
        point.x * sinA + point.y * cosA
    );
}

NAMESPACE_UPP_END

#endif