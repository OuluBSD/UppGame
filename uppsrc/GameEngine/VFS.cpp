#include "VFS.h"
#include <plugin/png/png.h>
#include <plugin/jpg/jpg.h>
#include <plugin/wav/Wav.h>

NAMESPACE_UPP

// Global VFS instance
static VFS global_vfs;

VFS& GetVFS() {
	return global_vfs;
}

VFS::VFS() {
	// Initialize with default content directory
	content_dir = AppendFileName(GetExeDir(), "Content");
}

VFS::~VFS() {
	Uninitialize();
}

bool VFS::Initialize() {
	// Create the content directory if it doesn't exist
	if (!DirectoryExists(content_dir)) {
		CreateDirectory(content_dir);
	}
	
	// Mount the content directory to the root virtual path
	Mount("/", content_dir);
	
	return true;
}

void VFS::Uninitialize() {
	mount_points.Clear();
}

bool VFS::Mount(const String& virtual_path, const String& physical_path) {
	if (virtual_path.IsEmpty() || physical_path.IsEmpty()) {
		return false;
	}
	
	// Normalize the paths
	String norm_virtual = NormalizePath(virtual_path);
	String norm_physical = NormalizePath(physical_path);
	
	// Ensure physical path exists and is a directory
	if (!::DirectoryExists(norm_physical)) {
		// Try to create it if it doesn't exist
		if (!::CreateDirectory(norm_physical)) {
			LOG("Failed to mount " << norm_virtual << " to " << norm_physical << ", directory doesn't exist and couldn't be created");
			return false;
		}
	}
	
	// Add to mount points
	mount_points.Add(norm_virtual, norm_physical);
	
	// Sort mount points by length (descending) so longer paths match first
	mount_points.Sort(0, [](const String& a, const String& b) { return a.GetCount() > b.GetCount(); });
	
	return true;
}

bool VFS::Unmount(const String& virtual_path) {
	if (virtual_path.IsEmpty()) {
		return false;
	}
	
	String norm_virtual = NormalizePath(virtual_path);
	
	int index = mount_points.Find(norm_virtual);
	if (index >= 0) {
		mount_points.Remove(index);
		return true;
	}
	
	return false;
}

String VFS::FindPhysicalPath(const String& virtual_path) const {
	if (virtual_path.IsEmpty()) {
		return String();
	}
	
	String norm_path = NormalizePath(virtual_path);
	
	// Find the best matching mount point
	for (const auto& mount : mount_points) {
		const String& mount_point = mount.key;
		const String& physical_base = mount.value;
		
		// Check if the virtual path starts with this mount point
		if (norm_path.StartsWith(mount_point)) {
			// Calculate the relative path within the mount point
			String relative_path;
			if (norm_path.GetCount() > mount_point.GetCount()) {
				relative_path = norm_path.Mid(mount_point.GetCount());
				if (relative_path.StartsWith("/")) {
					relative_path = relative_path.Mid(1);
				}
			}
			
			// Combine the physical base with the relative path
			String physical_path = AppendFileName(physical_base, relative_path);
			return physical_path;
		}
	}
	
	// No mount point found, return empty string
	return String();
}

bool VFS::FileExists(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	return !physical_path.IsEmpty() && ::FileExists(physical_path);
}

bool VFS::DirectoryExists(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	return !physical_path.IsEmpty() && ::DirectoryExists(physical_path);
}

int64 VFS::GetFileSize(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	return ::GetFileLength(physical_path);
}

Time VFS::GetFileTimestamp(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	return GetFileTime(physical_path);
}

String VFS::LoadString(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty() || !::FileExists(physical_path)) {
		return String();
	}
	
	return LoadFile(physical_path);
}

Vector<byte> VFS::LoadBytes(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty() || !::FileExists(physical_path)) {
		return Vector<byte>();
	}
	
	return LoadFileBytes(physical_path);
}

bool VFS::SaveString(const String& path, const String& content) {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty()) {
		LOG("Could not resolve virtual path: " << path);
		return false;
	}
	
	// Create directories if needed
	if (!CreateDirectoriesForFile(physical_path)) {
		LOG("Could not create directories for file: " << physical_path);
		return false;
	}
	
	return SaveFile(physical_path, content);
}

bool VFS::SaveBytes(const String& path, const Vector<byte>& data) {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty()) {
		LOG("Could not resolve virtual path: " << path);
		return false;
	}
	
	// Create directories if needed
	if (!CreateDirectoriesForFile(physical_path)) {
		LOG("Could not create directories for file: " << physical_path);
		return false;
	}
	
	FileOut out(physical_path);
	if (out.IsError()) {
		LOG("Could not open file for writing: " << physical_path);
		return false;
	}
	
	out.Write(data.Begin(), data.GetCount());
	return !out.IsError();
}

Vector<FileInfo> VFS::ListDirectory(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty() || !::DirectoryExists(physical_path)) {
		return Vector<FileInfo>();
	}
	
	Vector<FileInfo> result;
	
	FindFile ff(AppendFileName(physical_path, "*.*"));
	while (ff.Find()) {
		FileInfo info;
		info.name = ff.GetName();
		info.size = ff.GetLength();
		info.timestamp = ff.GetLastWrite();
		info.is_directory = ff.IsDirectory();
		info.is_file = !ff.IsDirectory();
		
		result.Add(std::move(info));
	}
	
	return result;
}

bool VFS::CreateDirectory(const String& path) {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty()) {
		LOG("Could not resolve virtual path: " << path);
		return false;
	}
	
	return ::CreateDirectory(physical_path);
}

bool VFS::DeleteFile(const String& path) {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty() || !::FileExists(physical_path)) {
		LOG("File does not exist: " << physical_path);
		return false;
	}
	
	return ::DeleteFile(physical_path);
}

String VFS::GetRealPath(const String& virtual_path) const {
	return FindPhysicalPath(virtual_path);
}

String VFS::NormalizePath(const String& path) const {
	if (path.IsEmpty()) return path;
	
	String result = path;
	
	// Replace backslashes with forward slashes
	result = result.Replace('\\', '/');
	
	// Remove double slashes
	while (result.Find("//") >= 0) {
		result = result.Replace("//", "/");
	}
	
	// Handle relative paths (resolve ../ and ./)
	Vector<String> parts = Split(result, '/');
	Vector<String> resolved_parts;
	
	for (const String& part : parts) {
		if (part == ".") {
			// Current directory, skip
			continue;
		} else if (part == "..") {
			// Parent directory, go up one level if possible
			if (!resolved_parts.IsEmpty() && resolved_parts.Top() != "..") {
				resolved_parts.RemoveTop();
			} else {
				// Can't go above root, add as is
				resolved_parts.Add(part);
			}
		} else if (!part.IsEmpty()) {
			resolved_parts.Add(part);
		}
	}
	
	// Combine the resolved parts
	result = Join(resolved_parts, '/');
	
	// Ensure leading slash for absolute paths
	if (path.StartsWith("/")) {
		result = "/" + result;
	}
	
	return result;
}

String VFS::GetExtension(const String& path) const {
	return GetFileExt(path);
}

String VFS::GetFileName(const String& path) const {
	String norm_path = NormalizePath(path);
	int last_slash = norm_path.ReverseFind('/');
	if (last_slash >= 0) {
		return norm_path.Mid(last_slash + 1);
	}
	return norm_path;
}

String VFS::GetDirectoryName(const String& path) const {
	String norm_path = NormalizePath(path);
	int last_slash = norm_path.ReverseFind('/');
	if (last_slash >= 0) {
		return norm_path.Left(last_slash);
	}
	return String();
}

bool VFS::IsAbsolutePath(const String& path) const {
	return path.StartsWith("/");
}

String VFS::CombinePaths(const String& base, const String& relative) const {
	String base_norm = NormalizePath(base);
	String rel_norm = NormalizePath(relative);
	
	if (rel_norm.StartsWith("/")) {
		return rel_norm;  // relative is actually absolute
	}
	
	if (base_norm.IsEmpty()) {
		return rel_norm;
	}
	
	if (base_norm.EndsWith("/")) {
		return base_norm + rel_norm;
	}
	
	return base_norm + "/" + rel_norm;
}

bool VFS::CreateDirectoriesForFile(const String& path) const {
	String dir = GetFileDirectory(path);
	if (!dir.IsEmpty() && !::DirectoryExists(dir)) {
		return ::CreateDirectoryDeep(dir);
	}
	return true;
}

Image VFS::LoadImage(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty() || !::FileExists(physical_path)) {
		return Image();
	}
	
	String ext = ToLower(GetFileExt(physical_path));
	
	if (ext == ".png") {
		PNGDecoder decoder;
		return decoder.LoadFile(physical_path);
	} else if (ext == ".jpg" || ext == ".jpeg") {
		JPEGDecoder decoder;
		return decoder.LoadFile(physical_path);
	}
	
	// Try to load with any available decoder
	Image img = StreamRaster::LoadFile(physical_path);
	return img;
}

SoundData VFS::LoadSound(const String& path) const {
	String physical_path = FindPhysicalPath(path);
	if (physical_path.IsEmpty() || !::FileExists(physical_path)) {
		return SoundData(); // Return empty sound data
	}
	
	String ext = ToLower(GetFileExt(physical_path));
	
	if (ext == ".wav") {
		WAVDecoder decoder;
		SoundData sound_data;
		if (decoder.LoadFile(physical_path, sound_data)) {
			return sound_data;
		}
	}
	
	// For now, return empty sound data for unsupported formats
	return SoundData();
}

END_UPP_NAMESPACE