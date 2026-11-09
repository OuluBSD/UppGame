#ifndef UPP_VFSYSTEM_H
#define UPP_VFSYSTEM_H

#include <Core/Core.h>
#include <GameLib/GameLib.h>

NAMESPACE_UPP

// File access flags
enum FileAccessMode {
	FILE_READ = 0x01,
	FILE_WRITE = 0x02,
	FILE_APPEND = 0x04,
	FILE_CREATE = 0x08,
	FILE_TRUNCATE = 0x10
};

// File information structure
struct FileInfo {
	String name;
	int64 size = 0;
	Time timestamp;
	bool is_directory = false;
	bool is_file = false;
	
	FileInfo() = default;
	FileInfo(const String& n, int64 s, Time t, bool dir, bool file) 
		: name(n), size(s), timestamp(t), is_directory(dir), is_file(file) {}
};

// Virtual File System for cross-platform asset management
class VFS {
public:
	VFS();
	virtual ~VFS();
	
	// Initialize the VFS
	bool Initialize();
	void Uninitialize();
	
	// Mount a physical directory to a virtual path
	bool Mount(const String& virtual_path, const String& physical_path);
	bool Unmount(const String& virtual_path);
	
	// File operations
	bool FileExists(const String& path) const;
	bool DirectoryExists(const String& path) const;
	int64 GetFileSize(const String& path) const;
	Time GetFileTimestamp(const String& path) const;
	
	// Read file content to string
	String LoadString(const String& path) const;
	
	// Read file content to memory
	Vector<byte> LoadBytes(const String& path) const;
	
	// Write string to file
	bool SaveString(const String& path, const String& content);
	
	// Write bytes to file
	bool SaveBytes(const String& path, const Vector<byte>& data);
	
	// List directory contents
	Vector<FileInfo> ListDirectory(const String& path) const;
	
	// Create directory
	bool CreateDirectory(const String& path);
	
	// Delete file
	bool DeleteFile(const String& path);
	
	// Get the real physical path for a virtual path
	String GetRealPath(const String& virtual_path) const;
	
	// Normalize path (resolve .. and .)
	String NormalizePath(const String& path) const;
	
	// Get file extension
	String GetExtension(const String& path) const;
	
	// Get file name without path
	String GetFileName(const String& path) const;
	
	// Get directory name from path
	String GetDirectoryName(const String& path) const;
	
	// Check if path is absolute
	bool IsAbsolutePath(const String& path) const;
	
	// Combine paths
	String CombinePaths(const String& base, const String& relative) const;
	
	// Asset loading helpers
	Image LoadImage(const String& path) const;
	SoundData LoadSound(const String& path) const;
	
	// Get/set content directory (for game assets)
	void SetContentDirectory(const String& path) { content_dir = path; }
	String GetContentDirectory() const { return content_dir; }

private:
	// Mapping from virtual paths to physical paths
	VectorMap<String, String> mount_points;
	
	// Content directory (default location for game assets)
	String content_dir;
	
	// Find the correct physical path for a virtual path
	String FindPhysicalPath(const String& virtual_path) const;
	
	// Utility function to create all necessary directories for a file path
	bool CreateDirectoriesForFile(const String& path) const;
};

// Global VFS instance
extern VFS& GetVFS();

END_UPP_NAMESPACE

#endif