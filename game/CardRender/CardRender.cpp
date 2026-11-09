#include "CardRender.h"

NAMESPACE_UPP

static String ResolveCardArtPath(const String& theme, const String& file_name)
{
	auto MakeRelativePath = [](const String& base_theme, const String& name) {
		return AppendFileName(AppendFileName("imgs", "cards"), AppendFileName(base_theme, name));
	};

	Vector<String> candidates;
	String requested_theme = TrimBoth(theme);
	if (requested_theme.IsEmpty())
		requested_theme = "default";

	auto AddCandidates = [&](const String& use_theme) {
		String rel = MakeRelativePath(use_theme, file_name);
		candidates.Add(ShareDirFile(rel));
		candidates.Add(AppendFileName(GetFileDirectory(GetExeFilePath()), AppendFileName("..", AppendFileName("share", rel))));
		candidates.Add(AppendFileName(GetCurrentDirectory(), AppendFileName("share", rel)));
	};

	AddCandidates(requested_theme);
	if (requested_theme != "default")
		AddCandidates("default");

	for (const String& candidate : candidates) {
		if (FileExists(candidate))
			return candidate;
	}
	return String();
}

Image LoadCardArt(const String& file_name, Size target_size, const String& theme)
{
	static VectorMap<String, Image> cache;
	String key = Format("%s|%s|%d|%d", TrimBoth(theme), file_name, target_size.cx, target_size.cy);
	int index = cache.Find(key);
	if (index >= 0)
		return cache[index];

	Image img;
	String path = ResolveCardArtPath(theme, file_name);
	if (!path.IsEmpty()) {
		img = StreamRaster::LoadFileAny(path);
		if (!img.IsEmpty() && target_size.cx > 0 && target_size.cy > 0)
			img = Rescale(img, target_size);
	}

	cache.Add(key, img);
	return img;
}

Image FitCardArt(const Image& img, Size target_size)
{
	if (img.IsEmpty() || target_size.cx <= 0 || target_size.cy <= 0)
		return img;

	Size source_size = img.GetSize();
	if (source_size.cx <= 0 || source_size.cy <= 0)
		return img;

	double ratio = min((double)target_size.cx / source_size.cx, (double)target_size.cy / source_size.cy);
	int target_cx = max(1, (int)(source_size.cx * ratio));
	int target_cy = max(1, (int)(source_size.cy * ratio));
	if (target_cx == source_size.cx && target_cy == source_size.cy)
		return img;
	return Rescale(img, target_cx, target_cy);
}

Image RotateCardArt(const Image& img, int rotation_deg)
{
	int rot = rotation_deg % 360;
	if (rot < 0)
		rot += 360;
	if (rot == 90)
		return RotateClockwise(img);
	if (rot == 180)
		return Rotate180(img);
	if (rot == 270)
		return RotateAntiClockwise(img);
	return img;
}

END_UPP_NAMESPACE
