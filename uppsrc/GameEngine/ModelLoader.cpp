#include "GameEngine.h"
#include "ModelLoader.h"

NAMESPACE_UPP

ModelLoader::ModelLoader() {
    // Initialize with default settings
}

std::shared_ptr<Model> ModelLoader::LoadModel(const String& filePath) {
    // Extract the file extension to determine the format
    String extension = GetFileExt(filePath).ToLower();
    if (extension.GetCount() > 0 && extension[0] != '.') {
        extension = "." + extension;
    }
    
    if (!IsFormatSupported(extension)) {
        LOG("Unsupported model format: " + extension);
        return nullptr;
    }
    
    // Try to load the file
    String contents = LoadFile(filePath);
    if (contents.IsEmpty()) {
        LOG("Failed to load file: " + filePath);
        return nullptr;
    }
    
    // Convert to byte vector
    Vector<byte> data;
    data.SetCount(contents.GetCount());
    for (int i = 0; i < contents.GetCount(); i++) {
        data[i] = contents[i];
    }
    
    return LoadModelFromData(data, extension);
}

std::shared_ptr<Model> ModelLoader::LoadModelFromVFS(const String& vfsPath) {
    if (!vfs) {
        LOG("VFS not set for ModelLoader");
        return nullptr;
    }
    
    Vector<byte> data = vfs->ReadFile(vfsPath);
    if (data.IsEmpty()) {
        LOG("Failed to load file from VFS: " + vfsPath);
        return nullptr;
    }
    
    String extension = GetFileExt(vfsPath).ToLower();
    if (extension.GetCount() > 0 && extension[0] != '.') {
        extension = "." + extension;
    }
    
    if (!IsFormatSupported(extension)) {
        LOG("Unsupported model format: " + extension);
        return nullptr;
    }
    
    return LoadModelFromData(data, extension);
}

std::shared_ptr<Model> ModelLoader::LoadModelFromData(const Vector<byte>& data, const String& extension) {
    if (data.IsEmpty()) {
        LOG("Model data is empty");
        return nullptr;
    }
    
    std::shared_ptr<Model> model = std::make_shared<Model>();
    
    if (extension == ".obj") {
        if (!ParseOBJ(data, *model)) {
            LOG("Failed to parse OBJ file");
            return nullptr;
        }
    } else if (extension == ".fbx") {
        if (!ParseFBX(data, *model)) {
            LOG("Failed to parse FBX file");
            return nullptr;
        }
    } else if (extension == ".gltf") {
        if (!ParseGLTF(data, *model)) {
            LOG("Failed to parse GLTF file");
            return nullptr;
        }
    } else if (extension == ".glb") {
        if (!ParseGLB(data, *model)) {
            LOG("Failed to parse GLB file");
            return nullptr;
        }
    } else if (extension == ".stl") {
        if (!ParseSTL(data, *model)) {
            LOG("Failed to parse STL file");
            return nullptr;
        }
    } else {
        LOG("Unsupported model format: " + extension);
        return nullptr;
    }
    
    // Calculate the bounding box after loading
    model->SetBoundingBox(model->CalculateBoundingBox());
    
    return model;
}

std::shared_ptr<Model> ModelLoader::LoadModelFromStream(Stream& stream, const String& extension) {
    // Read all data from stream
    Vector<byte> data;
    byte buffer[1024];
    
    // First determine the size
    stream.Seek(0, Stream::END);
    int64 size = stream.GetPos();
    stream.Seek(0, Stream::BEG);
    
    if (size <= 0) {
        LOG("Stream is empty or invalid");
        return nullptr;
    }
    
    data.SetCount(size);
    stream.Seek(0, Stream::BEG);
    stream.Read(data.Begin(), size);
    
    return LoadModelFromData(data, extension);
}

Vector<String> ModelLoader::GetSupportedFormats() const {
    return supportedExtensions;
}

bool ModelLoader::IsFormatSupported(const String& extension) const {
    String ext = extension.ToLower();
    for (const auto& supportedExt : supportedExtensions) {
        if (ext == supportedExt) {
            return true;
        }
    }
    return false;
}

bool ModelLoader::ParseOBJ(const Vector<byte>& data, Model& model) {
    String content((const char*)data.Begin(), data.GetCount());
    Vector<String> lines = Split(content, "\n");

    Vector<Point3> positions;
    Vector<Vector3> normals;
    Vector<Point2> texCoords;
    Vector<Vector<int>> faces;  // Stores faces as triplets of (posIdx, texIdx, normIdx)

    for (const String& line : lines) {
        String trimmed = TrimLeft(line);
        if (trimmed.StartsWith("v ")) {
            // Vertex position
            Vector<String> parts = Split(trimmed.Mid(2), " ", true);
            if (parts.GetCount() >= 3) {
                Point3 pos = Point3(ScanDouble(parts[0]), ScanDouble(parts[1]), ScanDouble(parts[2]));
                positions.Add(pos);
            }
        } else if (trimmed.StartsWith("vt ")) {
            // Texture coordinate
            Vector<String> parts = Split(trimmed.Mid(3), " ", true);
            if (parts.GetCount() >= 2) {
                Point2 texCoord = Point2(ScanDouble(parts[0]), ScanDouble(parts[1]));
                texCoords.Add(texCoord);
            }
        } else if (trimmed.StartsWith("vn ")) {
            // Normal
            Vector<String> parts = Split(trimmed.Mid(3), " ", true);
            if (parts.GetCount() >= 3) {
                Vector3 norm = Vector3(ScanDouble(parts[0]), ScanDouble(parts[1]), ScanDouble(parts[2]));
                normals.Add(norm);
            }
        } else if (trimmed.StartsWith("f ")) {
            // Face - each face can have 3 or more vertices
            Vector<String> parts = Split(trimmed.Mid(2), " ", true);
            Vector<int> face;

            for (int i = 0; i < parts.GetCount(); i++) {
                if (!parts[i].IsEmpty()) {
                    Vector<String> indices = Split(parts[i], "/", true);
                    if (indices.GetCount() >= 1) {
                        int posIdx = ScanInt(indices[0]);
                        if (posIdx < 0) posIdx = positions.GetCount() + posIdx + 1;  // Handle negative indices
                        else posIdx -= 1;  // OBJ uses 1-based indexing

                        int texIdx = -1;
                        if (indices.GetCount() >= 2 && !indices[1].IsEmpty()) {
                            texIdx = ScanInt(indices[1]);
                            if (texIdx < 0) texIdx = texCoords.GetCount() + texIdx + 1;
                            else texIdx -= 1;
                        }

                        int normIdx = -1;
                        if (indices.GetCount() >= 3 && !indices[2].IsEmpty()) {
                            normIdx = ScanInt(indices[2]);
                            if (normIdx < 0) normIdx = normals.GetCount() + normIdx + 1;
                            else normIdx -= 1;
                        }

                        // Store as triplet: position, texture, normal
                        face.Add(posIdx);
                        face.Add(texIdx);
                        face.Add(normIdx);
                    }
                }
            }

            if (face.GetCount() >= 9) {  // At least 3 vertices (9 indices)
                faces.Add(face);
            }
        }
    }

    // Create a single mesh from the parsed data
    Mesh mesh;
    HashMap<String, int> vertexMap;  // To avoid duplicate vertices

    for (const auto& face : faces) {
        // For OBJ files, faces can be triangles, quads, or polygons
        // We'll triangulate them by creating triangles from the first vertex to each edge
        if (face.GetCount() >= 9) {  // At least 3 vertices (9 indices: pos, tex, norm for each)
            int baseVertex = 0;
            for (int i = 3; i <= face.GetCount() - 3; i += 3) {
                if (baseVertex + 6 < face.GetCount()) {
                    // Triangle: vertex0, vertex1, vertex2
                    int v1_posIdx = face[baseVertex];
                    int v1_texIdx = face[baseVertex + 1];
                    int v1_normIdx = face[baseVertex + 2];

                    int v2_posIdx = face[i];
                    int v2_texIdx = face[i + 1];
                    int v2_normIdx = face[i + 2];

                    int v3_posIdx = face[i + 3];
                    int v3_texIdx = face[i + 4];
                    int v3_normIdx = face[i + 5];

                    // Create vertices
                    Point3 pos1 = (v1_posIdx >= 0 && v1_posIdx < positions.GetCount()) ? positions[v1_posIdx] : Point3(0, 0, 0);
                    Point3 pos2 = (v2_posIdx >= 0 && v2_posIdx < positions.GetCount()) ? positions[v2_posIdx] : Point3(0, 0, 0);
                    Point3 pos3 = (v3_posIdx >= 0 && v3_posIdx < positions.GetCount()) ? positions[v3_posIdx] : Point3(0, 0, 0);

                    Vector3 norm1 = (v1_normIdx >= 0 && v1_normIdx < normals.GetCount()) ? normals[v1_normIdx] : Vector3(0, 1, 0);
                    Vector3 norm2 = (v2_normIdx >= 0 && v2_normIdx < normals.GetCount()) ? normals[v2_normIdx] : Vector3(0, 1, 0);
                    Vector3 norm3 = (v3_normIdx >= 0 && v3_normIdx < normals.GetCount()) ? normals[v3_normIdx] : Vector3(0, 1, 0);

                    Point2 tex1 = (v1_texIdx >= 0 && v1_texIdx < texCoords.GetCount()) ? texCoords[v1_texIdx] : Point2(0, 0);
                    Point2 tex2 = (v2_texIdx >= 0 && v2_texIdx < texCoords.GetCount()) ? texCoords[v2_texIdx] : Point2(0, 0);
                    Point2 tex3 = (v3_texIdx >= 0 && v3_texIdx < texCoords.GetCount()) ? texCoords[v3_texIdx] : Point2(0, 0);

                    // Add triangle vertices to the mesh
                    mesh.AddVertex(Vertex(pos1, norm1, tex1));
                    mesh.AddVertex(Vertex(pos2, norm2, tex2));
                    mesh.AddVertex(Vertex(pos3, norm3, tex3));

                    baseVertex = i;
                }
            }
        }
    }

    // Add the mesh to the model
    model.AddMesh(mesh);

    return true;
}

bool ModelLoader::ParseFBX(const Vector<byte>& data, Model& model) {
    // FBX parsing is complex and typically requires specialized libraries
    // For this implementation, we'll return false as a placeholder
    LOG("FBX parsing not implemented in this version");
    return false;
}

bool ModelLoader::ParseGLTF(const Vector<byte>& data, Model& model) {
    // GLTF parsing would need JSON parsing and binary data handling
    // For this implementation, we'll return false as a placeholder
    LOG("GLTF parsing not implemented in this version");
    return false;
}

bool ModelLoader::ParseGLB(const Vector<byte>& data, Model& model) {
    // GLB parsing involves binary data with JSON header
    // For this implementation, we'll return false as a placeholder
    LOG("GLB parsing not implemented in this version");
    return false;
}

bool ModelLoader::ParseSTL(const Vector<byte>& data, Model& model) {
    // STL parsing would handle both ASCII and binary formats
    // For this implementation, we'll return false as a placeholder
    LOG("STL parsing not implemented in this version");
    return false;
}

END_UPP_NAMESPACE