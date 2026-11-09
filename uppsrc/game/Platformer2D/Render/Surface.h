#ifndef _Platformer2D_Render_Surface_h_
#define _Platformer2D_Render_Surface_h_

// Implementation header - included from Render.h (which already has NAMESPACE_UPP)

// ============================================================================
// IndexedSurface Implementation
// ============================================================================

inline IndexedSurface::IndexedSurface()
{
}

inline IndexedSurface::IndexedSurface(int width_, int height_)
{
	Create(width_, height_);
}

inline IndexedSurface::~IndexedSurface()
{
	Destroy();
}

inline void IndexedSurface::Create(int width_, int height_)
{
	Destroy();
	width = width_;
	height = height_;
	pitch = (width + 3) & ~3; // 4-byte alignment
	size_t size = pitch * height;
	data = (uint8_t*)MemoryAlloc(size);
	if (data)
	{
		memset(data, 0, size);
		owned = true;
	}
}

inline void IndexedSurface::Destroy()
{
	if (owned && data)
	{
		MemoryFree(data);
		data = nullptr;
	}
	width = height = pitch = 0;
	owned = false;
}

inline uint8_t IndexedSurface::GetPixel(int x, int y) const
{
	if (x < 0 || x >= width || y < 0 || y >= height) return 0;
	return data[y * pitch + x];
}

inline void IndexedSurface::SetPixel(int x, int y, uint8_t color)
{
	if (x >= 0 && x < width && y >= 0 && y < height)
		data[y * pitch + x] = color;
}

inline const uint8_t* IndexedSurface::GetRow(int y) const
{
	if (y < 0 || y >= height) return nullptr;
	return data + y * pitch;
}

inline uint8_t* IndexedSurface::GetRow(int y)
{
	if (y < 0 || y >= height) return nullptr;
	return data + y * pitch;
}

inline void IndexedSurface::Clear(uint8_t color)
{
	if (data)
		memset(data, color, pitch * height);
}

inline void IndexedSurface::FillRect(int x, int y, int w, int h, uint8_t color)
{
	if (!data) return;
	
	// Clamp
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > width) w = width - x;
	if (y + h > height) h = height - y;
	if (w <= 0 || h <= 0) return;
	
	uint8_t* row = data + y * pitch + x;
	for (int i = 0; i < h; ++i)
	{
		memset(row, color, w);
		row += pitch;
	}
}

inline void IndexedSurface::CopyFrom(const IndexedSurface& src, int dstX, int dstY, int srcX, int srcY, int w, int h)
{
	if (!data || !src.data) return;
	
	// Clamp to source
	if (srcX < 0) { w += srcX; dstX -= srcX; srcX = 0; }
	if (srcY < 0) { h += srcY; dstY -= srcY; srcY = 0; }
	if (srcX + w > src.width) w = src.width - srcX;
	if (srcY + h > src.height) h = src.height - srcY;
	
	// Clamp to destination
	if (dstX < 0) { w += dstX; srcX -= dstX; dstX = 0; }
	if (dstY < 0) { h += dstY; srcY -= dstY; dstY = 0; }
	if (dstX + w > width) w = width - dstX;
	if (dstY + h > height) h = height - dstY;
	if (w <= 0 || h <= 0) return;
	
	const uint8_t* srow = src.data + srcY * src.pitch + srcX;
	uint8_t* drow = data + dstY * pitch + dstX;
	for (int i = 0; i < h; ++i)
	{
		memcpy(drow, srow, w);
		srow += src.pitch;
		drow += pitch;
	}
}

inline void IndexedSurface::CopyTo(IndexedSurface& dst, int dstX, int dstY, int srcX, int srcY, int w, int h) const
{
	dst.CopyFrom(*this, dstX, dstY, srcX, srcY, w, h);
}

inline void IndexedSurface::Scroll(int dx, int dy)
{
	if (!data || (dx == 0 && dy == 0)) return;
	
	// Simple scroll by copying
	IndexedSurface temp(width, height);
	temp.CopyFrom(*this, 0, 0, 0, 0, width, height);
	
	Clear(0);
	CopyFrom(temp, dx, dy, 0, 0, width, height);
}


#endif // _Platformer2D_Render_Surface_h_