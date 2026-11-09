#ifndef UPP_MODELLOADER_H
#define UPP_MODELLOADER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/Model.h>
#include <GameEngine/VFS.h>
#include <Vector/Vector.h>

NAMESPACE_UPP

// Interface for loading 3D models from various file formats
class ModelLoader {
public:
    ModelLoader();
    virtual ~ModelLoader() = default;

    // Load a model from a file path
    std::shared_ptr<Model> LoadModel(const String& filePath);

    // Load a model from VFS path
    std::shared_ptr<Model> LoadModelFromVFS(const String& vfsPath);

    // Load from raw data
    std::shared_ptr<Model> LoadModelFromData(const Vector<byte>& data, const String& extension);

    // Load from stream
    std::shared_ptr<Model> LoadModelFromStream(Stream& stream, const String& extension);

    // Supported formats
    Vector<String> GetSupportedFormats() const;

    // Check if a format is supported
    bool IsFormatSupported(const String& extension) const;

    // Set VFS for file access
    void SetVFS(std::shared_ptr<VFS> vfs) { this->vfs = vfs; }
    std::shared_ptr<VFS> GetVFS() const { return vfs; }

protected:
    // Format-specific loading functions
    std::shared_ptr<Model> LoadOBJ(const Vector<byte>& data);
    std::shared_ptr<Model> LoadFBX(const Vector<byte>& data);
    std::shared_ptr<Model> LoadGLTF(const Vector<byte>& data);
    std::shared_ptr<Model> LoadGLB(const Vector<byte>& data);
    std::shared_ptr<Model> LoadSTL(const Vector<byte>& data);
    
    // Helper functions
    bool ParseOBJ(const Vector<byte>& data, Model& model);
    bool ParseFBX(const Vector<byte>& data, Model& model);
    bool ParseGLTF(const Vector<byte>& data, Model& model);
    bool ParseGLB(const Vector<byte>& data, Model& model);
    bool ParseSTL(const Vector<byte>& data, Model& model);

    // VFS for file access
    std::shared_ptr<VFS> vfs;

    // Supported extensions
    Vector<String> supportedExtensions = { ".obj", ".fbx", ".gltf", ".glb", ".stl" };
};

END_UPP_NAMESPACE

#endif