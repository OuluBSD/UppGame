#include "Umbrella.h"
#include "MapEditor.h"
#include "GameScreen.h"

#include <CtrlLib/CtrlLib.h>
#include <Draw/Draw.h>
#include <Core/Core.h>

using namespace Upp;

// Implementation of MapCanvas
MapCanvas::MapCanvas() {
	zoom = 1.0;
	offset = Point(0, 0);
	panning = false;
	panStart = Point(0, 0);
	cursorCol = -1;
	cursorRow = -1;
	showGrid = true;
	showReferenceImage = false;
	referenceImageOpacity = 50;
	referenceImageOffset = Point(0, 0);
	referenceImageScale = 1.0;
	referenceImagePanning = false;
	referenceImagePanStart = Point(0, 0);
	parentEditor = nullptr;
}

void MapCanvas::Paint(Draw& w) {
	Size sz = GetSize();

	// Get access to layer manager
	if(!parentEditor) {
		w.DrawRect(sz, Color(12, 17, 30));
		w.DrawText(10, 10, "Map Canvas - No parent editor", StdFont(), White());
		return;
	}

	LayerManager& layerMgr = parentEditor->GetLayerManager();

	// Draw canvas background (empty color)
	w.DrawRect(sz, Color(12, 17, 30));

	// Draw reference image if enabled
	if(showReferenceImage && !referenceImage.IsEmpty()) {
		// Scale reference image with both canvas zoom and reference image scale
		Size imgSize = referenceImage.GetSize();
		double totalScale = zoom * referenceImageScale;
		int scaledWidth = int(imgSize.cx * totalScale);
		int scaledHeight = int(imgSize.cy * totalScale);

		// Calculate position with both canvas offset and reference image offset
		int drawX = offset.x + int(referenceImageOffset.x * zoom);
		int drawY = offset.y + int(referenceImageOffset.y * zoom);

		// Apply opacity
		if(referenceImageOpacity < 100) {
			ImageDraw iw(imgSize);
			iw.DrawImage(0, 0, referenceImage);
			iw.Alpha().DrawRect(0, 0, imgSize.cx, imgSize.cy, GrayColor(referenceImageOpacity * 255 / 100));
			Image alphaImg = iw;
			w.DrawImage(drawX, drawY, scaledWidth, scaledHeight, alphaImg);
		} else {
			w.DrawImage(drawX, drawY, scaledWidth, scaledHeight, referenceImage);
		}
	}

	// Calculate tile size based on zoom
	int tileSize = int(14 * zoom);
	if(tileSize < 1) tileSize = 1;

	// Get map size from first layer (they should all be the same)
	int mapCols = 32;  // Default
	int mapRows = 24;
	if(layerMgr.GetLayerCount() > 0) {
		const MapGrid& grid = layerMgr.GetLayer(0).GetGrid();
		mapCols = grid.GetMapCols();
		mapRows = grid.GetMapRows();
	}

	// Calculate visible grid range
	int viewCols = sz.cx / tileSize + 2;
	int viewRows = sz.cy / tileSize + 2;

	int startCol = max(0, -offset.x / tileSize);
	int startRow = max(0, -offset.y / tileSize);
	int endCol = min(mapCols, startCol + viewCols);
	int endRow = min(mapRows, startRow + viewRows);

	// Render each visible layer (bottom to top)
	// Render in REVERSE order so Annotations (highest index) renders first (behind)
	for(int layerIndex = layerMgr.GetLayerCount() - 1; layerIndex >= 0; layerIndex--) {
		const Layer& layer = layerMgr.GetLayer(layerIndex);

		if(!layer.IsVisible()) continue;

		const MapGrid& grid = layer.GetGrid();
		int opacity = layer.GetOpacity();
		int gridRows = grid.GetRows();

		// Render tiles in this layer
		for(int row = startRow; row < endRow; row++) {
			for(int col = startCol; col < endCol; col++) {
				TileType tile = grid.GetTile(col, row);

				if(tile == TILE_EMPTY) continue;

				// Calculate screen position
				int screenX = col * tileSize + offset.x;
				int screenY = row * tileSize + offset.y;

				// Get tile color
				Color tileColor = TileTypeToColor(tile);

				// Apply layer opacity
				if(opacity < 100) {
					Color bgColor = Color(12, 17, 30);
					int alpha = opacity * 255 / 100;
					tileColor = Color(
						(tileColor.GetR() * alpha + bgColor.GetR() * (255 - alpha)) / 255,
						(tileColor.GetG() * alpha + bgColor.GetG() * (255 - alpha)) / 255,
						(tileColor.GetB() * alpha + bgColor.GetB() * (255 - alpha)) / 255
					);
				}

				// Draw tile as filled rectangle
				w.DrawRect(screenX, screenY, tileSize, tileSize, tileColor);
			}
		}
	}

	// Draw grid lines (on top of tiles) - only within map bounds
	if(showGrid) {
		Color gridColor = Color(51, 69, 92);

		// Calculate map boundaries in screen space
		int mapLeft = offset.x;
		int mapTop = offset.y;
		int mapRight = mapCols * tileSize + offset.x;
		int mapBottom = mapRows * tileSize + offset.y;

		// Vertical lines - clipped to map height
		for(int col = startCol; col <= endCol; col++) {
			int screenX = col * tileSize + offset.x;
			if(screenX >= 0 && screenX < sz.cx) {
				int y1 = max(0, mapTop);
				int y2 = min(sz.cy, mapBottom);
				if(y1 < y2) {
					w.DrawLine(screenX, y1, screenX, y2, 1, gridColor);
				}
			}
		}

		// Horizontal lines - clipped to map width
		for(int row = startRow; row <= endRow; row++) {
			int screenY = row * tileSize + offset.y;
			if(screenY >= 0 && screenY < sz.cy) {
				int x1 = max(0, mapLeft);
				int x2 = min(sz.cx, mapRight);
				if(x1 < x2) {
					w.DrawLine(x1, screenY, x2, screenY, 1, gridColor);
				}
			}
		}
	}

	// Draw tool preview
	if(cursorCol >= 0 && cursorRow >= 0 && parentEditor) {
		BrushTool& brush = parentEditor->GetBrushTool();
		MapEditorApp::EditTool currentTool = parentEditor->GetCurrentTool();

		if(currentTool == MapEditorApp::TOOL_FILL) {
			// Fill tool preview: show bucket icon
			int screenX = cursorCol * tileSize + offset.x;
			int screenY = cursorRow * tileSize + offset.y;

			// Yellow outline (2px thick)
			w.DrawRect(screenX, screenY, tileSize, tileSize, Null);
			w.DrawLine(screenX, screenY, screenX + tileSize, screenY, 2, Yellow());
			w.DrawLine(screenX + tileSize, screenY, screenX + tileSize, screenY + tileSize, 2, Yellow());
			w.DrawLine(screenX + tileSize, screenY + tileSize, screenX, screenY + tileSize, 2, Yellow());
			w.DrawLine(screenX, screenY + tileSize, screenX, screenY, 2, Yellow());

			// Show fill color preview
			FillTool& fill = parentEditor->GetFillTool();
			Color fillColor = TileTypeToColor(brush.GetPaintTile());
			int alpha = 180;
			Color bgColor = Color(12, 17, 30);
			fillColor = Color(
				(fillColor.GetR() * alpha + bgColor.GetR() * (255 - alpha)) / 255,
				(fillColor.GetG() * alpha + bgColor.GetG() * (255 - alpha)) / 255,
				(fillColor.GetB() * alpha + bgColor.GetB() * (255 - alpha)) / 255
			);
			w.DrawRect(screenX + 2, screenY + 2, tileSize - 4, tileSize - 4, fillColor);
		}
		else {
			Vector<Point> brushTiles;
			brush.GetBrushTiles(cursorCol, cursorRow, brushTiles);

			// Draw preview for each tile
			for(const Point& pt : brushTiles) {
				if(pt.x < 0 || pt.x >= 100 || pt.y < 0 || pt.y >= 100) continue;

				int screenX = pt.x * tileSize + offset.x;
				int screenY = pt.y * tileSize + offset.y;

				if(currentTool == MapEditorApp::TOOL_ERASER) {
					// Draw red X for eraser
					w.DrawLine(screenX + 2, screenY + 2, screenX + tileSize - 2, screenY + tileSize - 2, 2, LtRed());
					w.DrawLine(screenX + tileSize - 2, screenY + 2, screenX + 2, screenY + tileSize - 2, 2, LtRed());

					// Red outline (2px thick)
					w.DrawLine(screenX, screenY, screenX + tileSize, screenY, 2, LtRed());
					w.DrawLine(screenX + tileSize, screenY, screenX + tileSize, screenY + tileSize, 2, LtRed());
					w.DrawLine(screenX + tileSize, screenY + tileSize, screenX, screenY + tileSize, 2, LtRed());
					w.DrawLine(screenX, screenY + tileSize, screenX, screenY, 2, LtRed());
				}
				else {
					// Brush preview: semi-transparent tile color
					Color previewColor = TileTypeToColor(brush.GetPaintTile());
					int alpha = 128;
					Color bgColor = Color(12, 17, 30);
					previewColor = Color(
						(previewColor.GetR() * alpha + bgColor.GetR() * (255 - alpha)) / 255,
						(previewColor.GetG() * alpha + bgColor.GetG() * (255 - alpha)) / 255,
						(previewColor.GetB() * alpha + bgColor.GetB() * (255 - alpha)) / 255
					);

					// Draw semi-transparent preview
					w.DrawRect(screenX + 1, screenY + 1, tileSize - 2, tileSize - 2, previewColor);

					// Draw outline (1px white)
					w.DrawRect(screenX, screenY, tileSize, tileSize, Null);
					w.DrawLine(screenX, screenY, screenX + tileSize, screenY, 1, White());
					w.DrawLine(screenX + tileSize - 1, screenY, screenX + tileSize - 1, screenY + tileSize, 1, White());
					w.DrawLine(screenX + tileSize - 1, screenY + tileSize - 1, screenX, screenY + tileSize - 1, 1, White());
					w.DrawLine(screenX, screenY + tileSize - 1, screenX, screenY, 1, White());
				}
			}
		}
	}

	// Render spawn points (always on top)
	MapEditorApp::EditTool activeTool = parentEditor->GetCurrentTool();
	if(activeTool == MapEditorApp::TOOL_ENEMY_PLACEMENT ||
	   activeTool == MapEditorApp::TOOL_DROPLET_PLACEMENT ||
	   activeTool == MapEditorApp::TOOL_PICKUP_PLACEMENT) {
		// Render active tool's spawn points and preview
		if(activeTool == MapEditorApp::TOOL_ENEMY_PLACEMENT) {
			parentEditor->GetEnemyTool().Render(w, cursorCol, cursorRow, offset, zoom, 14);
		}
		else if(activeTool == MapEditorApp::TOOL_DROPLET_PLACEMENT) {
			parentEditor->GetDropletTool().Render(w, cursorCol, cursorRow, offset, zoom, 14);
		}
		else {
			parentEditor->GetPickupTool().Render(w, cursorCol, cursorRow, offset, zoom, 14);
		}
	}
	else {
		// When not in entity placement mode, still show spawn points but without preview
		parentEditor->GetEnemyTool().Render(w, -1, -1, offset, zoom, 14);
		parentEditor->GetDropletTool().Render(w, -1, -1, offset, zoom, 14);
		parentEditor->GetPickupTool().Render(w, -1, -1, offset, zoom, 14);
	}
}

void MapCanvas::MouseMove(Point pos, dword flags) {
	if(referenceImagePanning) {
		// Update reference image offset (in unzoomed coordinates)
		Point delta = pos - referenceImagePanStart;
		referenceImageOffset.x += delta.x / zoom;
		referenceImageOffset.y += delta.y / zoom;
		referenceImagePanStart = pos;
		Refresh();
	}
	else if(panning) {
		// Update camera offset
		Point delta = pos - panStart;
		offset += delta;
		panStart = pos;
		Refresh();
	}

	// Update cursor tile position
	int tileSize = int(14 * zoom);
	if(tileSize > 0) {
		cursorCol = (pos.x - offset.x) / tileSize;
		cursorRow = (pos.y - offset.y) / tileSize;

		// If painting, continue painting
		if((flags & K_MOUSELEFT) && parentEditor) {
			MapEditorApp::EditTool tool = parentEditor->GetCurrentTool();
			if(tool == MapEditorApp::TOOL_BRUSH || tool == MapEditorApp::TOOL_ERASER) {
				BrushTool& brush = parentEditor->GetBrushTool();
				LayerManager& layerMgr = parentEditor->GetLayerManager();

				if(tool == MapEditorApp::TOOL_BRUSH) {
					brush.SetMode(BRUSH_MODE_PAINT);
				} else {
					brush.SetMode(BRUSH_MODE_ERASE);
				}

				brush.ContinuePainting(cursorCol, cursorRow, layerMgr);
			}
		}

		// If right-click erasing, continue erasing
		if((flags & K_MOUSERIGHT) && parentEditor) {
			BrushTool& brush = parentEditor->GetBrushTool();
			LayerManager& layerMgr = parentEditor->GetLayerManager();

			brush.SetMode(BRUSH_MODE_ERASE);
			brush.ContinuePainting(cursorCol, cursorRow, layerMgr);
		}
	}

	Refresh();
}

void MapCanvas::LeftDown(Point pos, dword flags) {
	if(!parentEditor) return;

	// Get tile coordinates
	int tileSize = int(14 * zoom);
	if(tileSize <= 0) return;

	int col = (pos.x - offset.x) / tileSize;
	int row = (pos.y - offset.y) / tileSize;

	BrushTool& brush = parentEditor->GetBrushTool();
	LayerManager& layerMgr = parentEditor->GetLayerManager();

	// Check current tool
	if(parentEditor->GetCurrentTool() == MapEditorApp::TOOL_BRUSH) {
		brush.SetMode(BRUSH_MODE_PAINT);
		parentEditor->SnapshotActiveLayer();
		brush.StartPainting(col, row, layerMgr);
		Refresh();
	}
	else if(parentEditor->GetCurrentTool() == MapEditorApp::TOOL_ERASER) {
		brush.SetMode(BRUSH_MODE_ERASE);
		parentEditor->SnapshotActiveLayer();
		brush.StartPainting(col, row, layerMgr);
		Refresh();
	}
	else if(parentEditor->GetCurrentTool() == MapEditorApp::TOOL_FILL) {
		FillTool& fill = parentEditor->GetFillTool();

		// Synchronize fill tile with brush tile
		fill.SetFillTile(brush.GetPaintTile());

		parentEditor->SnapshotActiveLayer();
		fill.Fill(col, row, layerMgr);
		parentEditor->CommitUndo("Fill");
		Refresh();
	}
	else if(parentEditor->GetCurrentTool() == MapEditorApp::TOOL_ENEMY_PLACEMENT) {
		EnemyPlacementTool& enemyTool = parentEditor->GetEnemyTool();
		enemyTool.Click(col, row);
		Refresh();
	}
	else if(parentEditor->GetCurrentTool() == MapEditorApp::TOOL_DROPLET_PLACEMENT) {
		DropletPlacementTool& dropletTool = parentEditor->GetDropletTool();
		dropletTool.Click(col, row);
		Refresh();
	}
	else if(parentEditor->GetCurrentTool() == MapEditorApp::TOOL_PICKUP_PLACEMENT) {
		PickupPlacementTool& pickupTool = parentEditor->GetPickupTool();
		pickupTool.Click(col, row);
		Refresh();
	}
}

void MapCanvas::LeftUp(Point pos, dword flags) {
	if(!parentEditor) return;

	MapEditorApp::EditTool tool = parentEditor->GetCurrentTool();
	if(tool == MapEditorApp::TOOL_BRUSH) {
		BrushTool& brush = parentEditor->GetBrushTool();
		brush.StopPainting();
		parentEditor->CommitUndo("Paint");
	}
	else if(tool == MapEditorApp::TOOL_ERASER) {
		BrushTool& brush = parentEditor->GetBrushTool();
		brush.StopPainting();
		parentEditor->CommitUndo("Erase");
	}
}

void MapCanvas::RightDown(Point pos, dword flags) {
	if(!parentEditor) return;

	int tileSize = int(14 * zoom);
	if(tileSize <= 0) return;

	int col = (pos.x - offset.x) / tileSize;
	int row = (pos.y - offset.y) / tileSize;

	MapEditorApp::EditTool tool = parentEditor->GetCurrentTool();

	// Right-click in entity modes = remove entity at that cell
	if(tool == MapEditorApp::TOOL_ENEMY_PLACEMENT) {
		EnemyPlacementTool& et = parentEditor->GetEnemyTool();
		et.SetMode(PLACEMENT_REMOVE);
		et.Click(col, row);
		et.SetMode(PLACEMENT_ADD);
		Refresh();
		return;
	}
	if(tool == MapEditorApp::TOOL_DROPLET_PLACEMENT) {
		DropletPlacementTool& dt = parentEditor->GetDropletTool();
		dt.SetMode(PLACEMENT_REMOVE);
		dt.Click(col, row);
		dt.SetMode(PLACEMENT_ADD);
		Refresh();
		return;
	}
	if(tool == MapEditorApp::TOOL_PICKUP_PLACEMENT) {
		PickupPlacementTool& pt = parentEditor->GetPickupTool();
		pt.SetMode(PLACEMENT_REMOVE);
		pt.Click(col, row);
		pt.SetMode(PLACEMENT_ADD);
		Refresh();
		return;
	}

	// All other tools: right-click erases tiles
	BrushTool& brush = parentEditor->GetBrushTool();
	LayerManager& layerMgr = parentEditor->GetLayerManager();

	brush.SetMode(BRUSH_MODE_ERASE);
	parentEditor->SnapshotActiveLayer();
	brush.StartPainting(col, row, layerMgr);
	Refresh();
}

void MapCanvas::RightUp(Point pos, dword flags) {
	if(!parentEditor) return;

	MapEditorApp::EditTool tool = parentEditor->GetCurrentTool();
	if(tool == MapEditorApp::TOOL_ENEMY_PLACEMENT ||
	   tool == MapEditorApp::TOOL_DROPLET_PLACEMENT ||
	   tool == MapEditorApp::TOOL_PICKUP_PLACEMENT)
		return;

	BrushTool& brush = parentEditor->GetBrushTool();
	brush.StopPainting();
	parentEditor->CommitUndo("Erase");
	brush.SetMode(BRUSH_MODE_PAINT);  // Restore paint mode
}

void MapCanvas::MiddleDown(Point pos, dword flags) {
	// Ctrl+Middle drag = pan reference image, otherwise pan canvas
	if(flags & K_CTRL) {
		referenceImagePanning = true;
		referenceImagePanStart = pos;
	}
	else {
		panning = true;
		panStart = pos;
	}
	SetCapture();
}

void MapCanvas::MiddleUp(Point pos, dword flags) {
	panning = false;
	referenceImagePanning = false;
	ReleaseCapture();
}

void MapCanvas::MouseWheel(Point pos, int zdelta, dword flags) {
	// Ctrl+Wheel = scale reference image, otherwise zoom canvas
	if(flags & K_CTRL) {
		if(zdelta > 0) {
			referenceImageScale *= 1.1;
		} else {
			referenceImageScale /= 1.1;
		}
		if(referenceImageScale < 0.1) referenceImageScale = 0.1;
		if(referenceImageScale > 5.0) referenceImageScale = 5.0;
	}
	else {
		// Handle zoom with mouse wheel
		if(zdelta > 0) {
			zoom *= 1.1;
		} else {
			zoom /= 1.1;
		}
		if(zoom < 0.1) zoom = 0.1;
		if(zoom > 5.0) zoom = 5.0;
	}

	Refresh();
}

void MapCanvas::SetZoom(double newZoom) {
	zoom = newZoom;
	if(zoom < 0.1) zoom = 0.1;
	if(zoom > 5.0) zoom = 5.0;
	Refresh();
}

void MapCanvas::PanTo(Point newOffset) {
	offset = newOffset;
	Refresh();
}

void MapCanvas::SetReferenceImage(const Image& img) {
	referenceImage = img;
	Refresh();
}

void MapCanvas::ZoomToFit() {
	if(!parentEditor) return;

	Layer* layer = parentEditor->GetLayerManager().GetActiveLayer();
	if(!layer) return;

	const MapGrid& grid = layer->GetGrid();
	int mapCols = grid.GetMapCols();
	int mapRows = grid.GetMapRows();
	int tileSize = grid.GetGridSize();

	// Calculate zoom to fit map in canvas
	Size canvasSize = GetSize();
	double zoomX = double(canvasSize.cx) / (mapCols * tileSize);
	double zoomY = double(canvasSize.cy) / (mapRows * tileSize);

	zoom = min(zoomX, zoomY);
	offset = Point(0, 0);

	Refresh();
}

// Implementation of MapEditorApp
MapEditorApp::MapEditorApp() {
	Title("Umbrella Map Editor");
	Sizeable().MaximizeBox();
	SetRect(0, 0, 1400, 900);

	// Initialize tool
	currentTool = TOOL_BRUSH;

	// Connect entity tools to spawn arrays
	enemyTool.SetEnemySpawns(&enemySpawns);
	dropletTool.SetDropletSpawns(&dropletSpawns);
	pickupTool.SetPickupSpawns(&pickupSpawns);

	AddFrame(mainMenuBar);
	AddFrame(mainToolBar);
	AddFrame(mainStatusBar);

	// Initialize layer manager with default 100x100 grid
	layerManager.InitializeDefaultLayers(100, 100);

	// Add some test tiles for visual verification
	Layer* terrain = layerManager.GetActiveLayer();
	if(terrain) {
		terrain->GetGrid().SetTile(5, 5, TILE_WALL);
		terrain->GetGrid().SetTile(6, 5, TILE_WALL);
		terrain->GetGrid().SetTile(7, 5, TILE_FULLBLOCK);
		terrain->GetGrid().SetTile(5, 6, TILE_BACKGROUND);
		terrain->GetGrid().SetTile(6, 6, TILE_BACKGROUND);
		terrain->GetGrid().SetTile(7, 6, TILE_WALL);
	}

	mainMenuBar.Set(callback(this, &MapEditorApp::SetupMenuBar));
	SetupToolBar();
	SetupToolsPanel();
	SetupLayersPanel();
	SetupPropertiesPanel();

	// Load recent files
	LoadRecentFiles();

	// Set up canvas - add it to the window so it appears in the center
	mapCanvas.SetFrame(InsetFrame());
	mapCanvas.SetParentEditor(this);
	Add(mapCanvas.SizePos());
}

MapEditorApp::MapEditorApp(const String& levelPath) : MapEditorApp() {
	// Load the level after initialization
	if(FileExists(levelPath)) {
		OpenFile(levelPath);
	} else {
		Exclamation("Level file not found: " + levelPath);
	}
}

void MapEditorApp::SetupMenuBar(Bar& bar) {
	bar.Sub("File", callback(this, &MapEditorApp::SetupFileMenu));
	bar.Sub("Edit", callback(this, &MapEditorApp::SetupEditMenu));
	bar.Sub("View", callback(this, &MapEditorApp::SetupViewMenu));
}

void MapEditorApp::SetupFileMenu(Bar& bar) {
	bar.Add("New", CtrlImg::new_doc(), callback(this, &MapEditorApp::NewMapAction))
		.Key(K_CTRL_N)
		.Help("Create a new map");
	bar.Add("Open...", CtrlImg::open(), callback(this, &MapEditorApp::OpenFileAction))
		.Key(K_CTRL_O)
		.Help("Open an existing map file");
	bar.Add("Save", CtrlImg::save(), callback(this, &MapEditorApp::SaveFileAction))
		.Key(K_CTRL_S)
		.Help("Save the current map");

	// Recent files
	if(recentFiles.GetCount() > 0) {
		bar.Separator();
		for(int i = 0; i < recentFiles.GetCount(); i++) {
			String fileName = GetFileName(recentFiles[i]);
			bar.Add(fileName, [=] { OpenRecentFile(i); })
				.Help("Open " + recentFiles[i]);
		}
	}

	bar.Separator();
	bar.Add("Exit", callback(this, &MapEditorApp::ExitAction))
		.Key(K_ALT_F4)
		.Help("Exit the map editor");
}

void MapEditorApp::SetupEditMenu(Bar& bar) {
	bar.Add("Undo", CtrlImg::undo(), callback(this, &MapEditorApp::UndoAction))
		.Key(K_CTRL_Z)
		.Help("Undo the last action");
	bar.Add("Redo", CtrlImg::redo(), callback(this, &MapEditorApp::RedoAction))
		.Key(K_CTRL_Y)
		.Help("Redo the last undone action");
	bar.Separator();
	bar.Sub("Background", callback(this, &MapEditorApp::SetupBackgroundMenu));
}

void MapEditorApp::SetupBackgroundMenu(Bar& bar) {
	bar.Add("Load Reference Image...", callback(this, &MapEditorApp::BrowseReferenceImage))
		.Key(K_CTRL_R)
		.Help("Load reference image for tracing");
	bar.Add("Show Reference Image", [=] { mapCanvas.SetShowReferenceImage(!mapCanvas.GetShowReferenceImage()); })
		.Check(mapCanvas.GetShowReferenceImage())
		.Key(K_R)
		.Help("Toggle reference image visibility");
	bar.Separator();
	bar.Add("Pan Reference Image", [=] { PromptOK("Hold Ctrl+Middle-drag to pan reference image"); })
		.Help("Ctrl+Middle-drag to move reference image");
	bar.Add("Scale Reference Image", [=] { PromptOK("Hold Ctrl+Mouse-wheel to scale reference image"); })
		.Help("Ctrl+Mouse-wheel to scale reference image");
}

void MapEditorApp::SetupViewMenu(Bar& bar) {
	bar.Add("Zoom In", callback(this, &MapEditorApp::ZoomInAction))
		.Key(K_PLUS)
		.Help("Zoom in on the map");
	bar.Add("Zoom Out", callback(this, &MapEditorApp::ZoomOutAction))
		.Key(K_MINUS)
		.Help("Zoom out from the map");
	bar.Add("Reset Zoom", callback(this, &MapEditorApp::ResetZoomAction))
		.Key(K_CTRL_0)
		.Help("Reset zoom to 100%");
	bar.Separator();
	bar.Add("Show Grid", [=] { mapCanvas.SetShowGrid(!mapCanvas.GetShowGrid()); })
		.Check(mapCanvas.GetShowGrid())
		.Key(K_G)
		.Help("Toggle grid visibility");
	bar.Add("Show Reference Image", [=] { mapCanvas.SetShowReferenceImage(!mapCanvas.GetShowReferenceImage()); })
		.Check(mapCanvas.GetShowReferenceImage())
		.Key(K_R)
		.Help("Toggle reference image visibility");
	bar.Separator();
	bar.Add("Load Reference Image...", callback(this, &MapEditorApp::BrowseReferenceImage))
		.Key(K_CTRL_R)
		.Help("Load reference image for tracing");
}

void MapEditorApp::SetupToolBar() {
	// Helper lambda to explicitly pad icons to 16x16
	auto PadIcon = [](const Image& img) -> Image {
		// Always create 16x16 image regardless of input size
		ImageDraw iw(16, 16);
		iw.DrawRect(0, 0, 16, 16, SColorFace());
		Size sz = img.GetSize();
		int x = (16 - sz.cx) / 2;
		int y = (16 - sz.cy) / 2;
		iw.DrawImage(x, y, img);
		return iw;
	};

	// Set up the toolbar with common actions - use explicit 16x16 icons
	newMapBtn.SetImage(PadIcon(CtrlImg::new_doc())).Tip("New Map");
	newMapBtn <<= callback(this, &MapEditorApp::NewMapAction);
	mainToolBar.Add(newMapBtn, Size(20, 20));  // Button slightly larger than icon

	openFileBtn.SetImage(PadIcon(CtrlImg::open())).Tip("Open Map");
	openFileBtn <<= callback(this, &MapEditorApp::OpenFileAction);
	mainToolBar.Add(openFileBtn, Size(20, 20));

	saveFileBtn.SetImage(PadIcon(CtrlImg::save())).Tip("Save Map");
	saveFileBtn <<= callback(this, &MapEditorApp::SaveFileAction);
	mainToolBar.Add(saveFileBtn, Size(20, 20));

	mainToolBar.Separator();

	undoBtn.SetImage(PadIcon(CtrlImg::undo())).Tip("Undo");
	undoBtn <<= callback(this, &MapEditorApp::UndoAction);
	mainToolBar.Add(undoBtn, Size(20, 20));

	redoBtn.SetImage(PadIcon(CtrlImg::redo())).Tip("Redo");
	redoBtn <<= callback(this, &MapEditorApp::RedoAction);
	mainToolBar.Add(redoBtn, Size(20, 20));

	mainToolBar.Separator();

	zoomInBtn.SetImage(PadIcon(CtrlImg::plus())).Tip("Zoom In");
	zoomInBtn <<= callback(this, &MapEditorApp::ZoomInAction);
	mainToolBar.Add(zoomInBtn, Size(20, 20));

	zoomOutBtn.SetImage(PadIcon(CtrlImg::minus())).Tip("Zoom Out");
	zoomOutBtn <<= callback(this, &MapEditorApp::ZoomOutAction);
	mainToolBar.Add(zoomOutBtn, Size(20, 20));

	resetZoomBtn.SetLabel("100%").Tip("Reset Zoom");
	resetZoomBtn <<= callback(this, &MapEditorApp::ResetZoomAction);
	mainToolBar.Add(resetZoomBtn, Size(40, 20));  // Text button needs more width
}

void MapEditorApp::DockInit() {
	// Set up dockable panels using DockWindow API
	// Canvas was already added in constructor, panels dock around it
	DockLeft(Dockable(toolsPanel, "Tools").SizeHint(Size(250, 400)));
	DockRight(Dockable(layersPanel, "Layers").SizeHint(Size(250, 300)));
	DockRight(Dockable(entitiesPanel, "Entities").SizeHint(Size(250, 300)));
	DockBottom(Dockable(propertiesPanel, "Properties").SizeHint(Size(400, 200)));

	// Set initial status
	mainStatusBar.Set("Ready");
}

void MapEditorApp::SetupToolsPanel() {
	toolsPanel.SetFrame(InsetFrame());

	int yPos = 10;

	// Tools title
	toolsLabel.SetText("TOOLS");
	toolsLabel.SetAlign(ALIGN_CENTER);
	toolsPanel.Add(toolsLabel.HSizePos(10, 10).TopPos(yPos, 20));
	yPos += 30;

	// Brush size section
	brushSizeLabel.SetText("Brush Size:");
	toolsPanel.Add(brushSizeLabel.LeftPos(10, 100).TopPos(yPos, 20));
	yPos += 25;

	brush1x1Btn.SetLabel("1x1");
	brush1x1Btn << [=] {
		brushTool.SetBrushSize(BRUSH_1X1);
		mainStatusBar.Set("Brush size: 1x1");
	};
	toolsPanel.Add(brush1x1Btn.LeftPos(10, 50).TopPos(yPos, 25));

	brush2x2Btn.SetLabel("2x2");
	brush2x2Btn << [=] {
		brushTool.SetBrushSize(BRUSH_2X2);
		mainStatusBar.Set("Brush size: 2x2");
	};
	toolsPanel.Add(brush2x2Btn.LeftPos(70, 50).TopPos(yPos, 25));
	yPos += 30;

	brush3x3Btn.SetLabel("3x3");
	brush3x3Btn << [=] {
		brushTool.SetBrushSize(BRUSH_3X3);
		mainStatusBar.Set("Brush size: 3x3");
	};
	toolsPanel.Add(brush3x3Btn.LeftPos(10, 50).TopPos(yPos, 25));

	brush5x5Btn.SetLabel("5x5");
	brush5x5Btn << [=] {
		brushTool.SetBrushSize(BRUSH_5X5);
		mainStatusBar.Set("Brush size: 5x5");
	};
	toolsPanel.Add(brush5x5Btn.LeftPos(70, 50).TopPos(yPos, 25));
	yPos += 40;

	// Tile type section
	tileTypeLabel.SetText("Tile Type:");
	toolsPanel.Add(tileTypeLabel.LeftPos(10, 100).TopPos(yPos, 20));
	yPos += 25;

	wallBtn.SetLabel("Wall");
	wallBtn << [=] {
		brushTool.SetPaintTile(TILE_WALL);
		mainStatusBar.Set("Paint tile: Wall");
	};
	toolsPanel.Add(wallBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 30;

	bgBtn.SetLabel("Background");
	bgBtn << [=] {
		brushTool.SetPaintTile(TILE_BACKGROUND);
		mainStatusBar.Set("Paint tile: Background");
	};
	toolsPanel.Add(bgBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 30;

	blockBtn.SetLabel("Full Block");
	blockBtn << [=] {
		brushTool.SetPaintTile(TILE_FULLBLOCK);
		mainStatusBar.Set("Paint tile: Full Block");
	};
	toolsPanel.Add(blockBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 40;

	// Tool selection section
	toolSelectionLabel.SetText("Tool:");
	toolsPanel.Add(toolSelectionLabel.LeftPos(10, 100).TopPos(yPos, 20));
	yPos += 25;

	brushToolBtn.SetLabel("Brush");
	brushToolBtn << [=] {
		currentTool = TOOL_BRUSH;
		brushTool.SetMode(BRUSH_MODE_PAINT);
		mainStatusBar.Set("Tool: Brush");
	};
	toolsPanel.Add(brushToolBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 30;

	eraserToolBtn.SetLabel("Eraser");
	eraserToolBtn << [=] {
		currentTool = TOOL_ERASER;
		brushTool.SetMode(BRUSH_MODE_ERASE);
		mainStatusBar.Set("Tool: Eraser");
	};
	toolsPanel.Add(eraserToolBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 30;

	fillToolBtn.SetLabel("Fill");
	fillToolBtn << [=] {
		currentTool = TOOL_FILL;
		mainStatusBar.Set("Tool: Fill");
	};
	toolsPanel.Add(fillToolBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 40;

	// Entity Placement section
	Label& entLabel = *new Label();
	entLabel.SetText("Entities:");
	toolsPanel.Add(entLabel.LeftPos(10, 100).TopPos(yPos, 20));
	yPos += 25;

	enemyPlacementBtn.SetLabel("Enemy [5]");
	enemyPlacementBtn << [=] {
		currentTool = TOOL_ENEMY_PLACEMENT;
		enemyTool.SetMode(PLACEMENT_ADD);
		mainStatusBar.Set("Tool: Enemy Placement (5=add, right-click=remove)");
		mapCanvas.Refresh();
	};
	toolsPanel.Add(enemyPlacementBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 30;

	dropletPlacementBtn.SetLabel("Droplet [6]");
	dropletPlacementBtn << [=] {
		currentTool = TOOL_DROPLET_PLACEMENT;
		dropletTool.SetMode(PLACEMENT_ADD);
		mainStatusBar.Set("Tool: Droplet Placement (6=add, right-click=remove)");
		mapCanvas.Refresh();
	};
	toolsPanel.Add(dropletPlacementBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 30;

	pickupPlacementBtn.SetLabel("Pickup [7]");
	pickupPlacementBtn << [=] {
		currentTool = TOOL_PICKUP_PLACEMENT;
		pickupTool.SetMode(PLACEMENT_ADD);
		mainStatusBar.Set("Tool: Pickup Placement (7=add, right-click=remove)");
		mapCanvas.Refresh();
	};
	toolsPanel.Add(pickupPlacementBtn.LeftPos(10, 110).TopPos(yPos, 25));
	yPos += 40;

	// Enemy type selector
	enemyTypeLabel.SetText("Enemy Type:");
	toolsPanel.Add(enemyTypeLabel.LeftPos(10, 110).TopPos(yPos, 20));
	yPos += 22;

	enemyTypeList.Add("Patroller");
	enemyTypeList.Add("Jumper");
	enemyTypeList.Add("Shooter");
	enemyTypeList.Add("Flyer");
	enemyTypeList.SetIndex(0);
	enemyTypeList.WhenAction = [=] {
		int idx = enemyTypeList.GetIndex();
		EnemyType types[] = { ENEMY_PATROLLER, ENEMY_JUMPER, ENEMY_SHOOTER, ENEMY_FLYER };
		if(idx >= 0 && idx < 4)
			enemyTool.SetSelectedType(types[idx]);
	};
	toolsPanel.Add(enemyTypeList.HSizePos(10, 10).TopPos(yPos, 22));
	yPos += 28;

	enemyFacingLabel.SetText("Facing:");
	toolsPanel.Add(enemyFacingLabel.LeftPos(10, 110).TopPos(yPos, 20));
	yPos += 22;

	enemyFacingList.Add("Right");
	enemyFacingList.Add("Left");
	enemyFacingList.SetIndex(0);
	enemyFacingList.WhenAction = [=] {
		enemyTool.SetSelectedFacing(enemyFacingList.GetIndex() == 0 ? 1 : -1);
	};
	toolsPanel.Add(enemyFacingList.HSizePos(10, 10).TopPos(yPos, 22));
	yPos += 40;

	// Droplet type selector
	dropletTypeLabel.SetText("Droplet Type:");
	toolsPanel.Add(dropletTypeLabel.LeftPos(10, 110).TopPos(yPos, 20));
	yPos += 22;

	dropletTypeList.Add("Rainbow");
	dropletTypeList.Add("Ice");
	dropletTypeList.Add("Fire");
	dropletTypeList.SetIndex(0);
	dropletTypeList.WhenAction = [=] {
		int idx = dropletTypeList.GetIndex();
		DropletType modes[] = { DROPLET_RAINBOW, DROPLET_ICE, DROPLET_FIRE };
		if(idx >= 0 && idx < 3)
			dropletTool.SetSelectedType(modes[idx]);
	};
	toolsPanel.Add(dropletTypeList.HSizePos(10, 10).TopPos(yPos, 22));
	yPos += 40;

	// Pickup type selector
	pickupTypeLabel.SetText("Pickup Type:");
	toolsPanel.Add(pickupTypeLabel.LeftPos(10, 110).TopPos(yPos, 20));
	yPos += 22;

	pickupTypeList.Add("Heart");
	pickupTypeList.Add("Gem");
	pickupTypeList.Add("Lightning");
	pickupTypeList.Add("Speed");
	pickupTypeList.SetIndex(0);
	pickupTypeList.WhenAction = [=] {
		int idx = pickupTypeList.GetIndex();
		PickupType types[] = { PU_HEART, PU_GEM, PU_LIGHTNING, PU_SPEED };
		if(idx >= 0 && idx < 4)
			pickupTool.SetSelectedType(types[idx]);
	};
	toolsPanel.Add(pickupTypeList.HSizePos(10, 10).TopPos(yPos, 22));
}

void MapEditorApp::SetupLayersPanel() {
	layersPanel.SetFrame(InsetFrame());

	int yPos = 10;

	// Layers tree list
	layersList.AddColumn("Layer", 150);
	layersList.AddColumn("Visible", 50).Ctrls<Option>();  // Checkbox for visibility
	layersList.NoHeader();
	layersList.WhenSel = [=] {
		// Update active layer when selection changes
		int id = layersList.GetCursor();
		if(id >= 0) {
			Value layerIdxVal = layersList.Get(id);  // Get the key value (layer index)
			if(!layerIdxVal.IsVoid()) {
				int layerIdx = (int)layerIdxVal;
				if(layerIdx >= 0 && layerIdx < layerManager.GetLayerCount()) {
					layerManager.SetActiveLayer(layerIdx);

					// Update opacity slider to match selected layer
					const Layer& layer = layerManager.GetLayer(layerIdx);
					layerOpacitySlider.SetData(layer.GetOpacity());
					layerOpacityLabel.SetText(Format("Opacity: %d%%", layer.GetOpacity()));

					mapCanvas.Refresh();
				}
			}
		}
	};
	layersList.WhenCtrlsAction = [=] {
		// Handle visibility checkbox changes
		for(int i = 0; i < layersList.GetCount(); i++) {
			Value layerIdxVal = layersList.Get(i);  // Get the key value (layer index)
			if(!layerIdxVal.IsVoid()) {
				int layerIdx = (int)layerIdxVal;

				// Root layer items
				if(layerIdx >= 0 && layerIdx < layerManager.GetLayerCount()) {
					bool visible = layersList.GetRowValue(i, 1);
					layerManager.GetLayer(layerIdx).SetVisible(visible);
				}
				// Reference image child item (layerIdx == -1)
				else if(layerIdx == -1) {
					bool visible = layersList.GetRowValue(i, 1);
					mapCanvas.SetShowReferenceImage(visible);
				}
			}
		}
		mapCanvas.Refresh();
	};
	layersPanel.Add(layersList.HSizePos(10, 10).TopPos(yPos, 200));
	yPos += 210;

	// Layer management buttons
	addLayerBtn.SetLabel("Add Layer");
	addLayerBtn << [=] {
		int cols = layerManager.GetLayerCount() > 0
			? layerManager.GetLayer(0).GetGrid().GetColumns()
			: max(1, (int)mapColsSpin.GetData());
		int rows = layerManager.GetLayerCount() > 0
			? layerManager.GetLayer(0).GetGrid().GetRows()
			: max(1, (int)mapRowsSpin.GetData());
		String name = "Layer " + AsString(layerManager.GetLayerCount() + 1);
		layerManager.AddLayer(name, LAYER_TERRAIN, cols, rows);
		undoStack.Clear();
		RefreshLayersList();
		mapCanvas.Refresh();
		mainStatusBar.Set("Added layer: " + name);
	};
	layersPanel.Add(addLayerBtn.LeftPos(10, 100).TopPos(yPos, 25));

	removeLayerBtn.SetLabel("Remove");
	removeLayerBtn << [=] {
		int id = layersList.GetCursor();
		if(id < 0) { mainStatusBar.Set("No layer selected."); return; }
		Value idxVal = layersList.Get(id);
		if(idxVal.IsVoid()) return;
		int layerIdx = (int)idxVal;
		if(layerIdx < 0 || layerIdx >= layerManager.GetLayerCount()) return;
		if(layerManager.GetLayerCount() <= 1) {
			PromptOK("Cannot remove the last layer.");
			return;
		}
		if(!PromptYesNo("Remove layer '" + layerManager.GetLayer(layerIdx).GetName() + "'?"))
			return;
		layerManager.RemoveLayer(layerIdx);
		undoStack.Clear();
		RefreshLayersList();
		mapCanvas.Refresh();
		mainStatusBar.Set("Layer removed.");
	};
	layersPanel.Add(removeLayerBtn.LeftPos(120, 100).TopPos(yPos, 25));
	yPos += 30;

	moveLayerUpBtn.SetLabel("Move Up");
	moveLayerUpBtn << [=] {
		int id = layersList.GetCursor();
		if(id < 0) return;
		Value idxVal = layersList.Get(id);
		if(idxVal.IsVoid()) return;
		int layerIdx = (int)idxVal;
		if(layerIdx <= 0 || layerIdx >= layerManager.GetLayerCount()) return;
		layerManager.MoveLayerUp(layerIdx);
		undoStack.Clear();
		RefreshLayersList();
		mapCanvas.Refresh();
	};
	layersPanel.Add(moveLayerUpBtn.LeftPos(10, 100).TopPos(yPos, 25));

	moveLayerDownBtn.SetLabel("Move Down");
	moveLayerDownBtn << [=] {
		int id = layersList.GetCursor();
		if(id < 0) return;
		Value idxVal = layersList.Get(id);
		if(idxVal.IsVoid()) return;
		int layerIdx = (int)idxVal;
		if(layerIdx < 0 || layerIdx >= layerManager.GetLayerCount() - 1) return;
		layerManager.MoveLayerDown(layerIdx);
		undoStack.Clear();
		RefreshLayersList();
		mapCanvas.Refresh();
	};
	layersPanel.Add(moveLayerDownBtn.LeftPos(120, 100).TopPos(yPos, 25));
	yPos += 40;

	// Layer opacity slider
	layerOpacityLabel.SetText("Opacity: 100%");
	layersPanel.Add(layerOpacityLabel.LeftPos(10, 100).TopPos(yPos, 20));
	yPos += 25;

	layerOpacitySlider.Range(100);
	layerOpacitySlider.MinMax(0, 100);
	layerOpacitySlider << [=] {
		int opacity = (int)layerOpacitySlider.GetData();
		layerOpacityLabel.SetText(Format("Opacity: %d%%", opacity));

		Layer* activeLayer = layerManager.GetActiveLayer();
		if(activeLayer) {
			activeLayer->SetOpacity(opacity);
			mapCanvas.Refresh();
		}
	};
	layersPanel.Add(layerOpacitySlider.HSizePos(10, 10).TopPos(yPos, 20));

	// Populate layers list
	RefreshLayersList();
}

void MapEditorApp::RefreshLayersList() {
	layersList.Clear();

	// Add layers with visibility checkboxes
	for(int i = 0; i < layerManager.GetLayerCount(); i++) {
		const Layer& layer = layerManager.GetLayer(i);

		// Add layer as root item
		// TreeArrayCtrl.Add(parentid, img, key_value, display_text)
		// The key_value (3rd param) is stored and retrieved with Get(id)
		// The display_text (4th param) is what shows in the tree
		int id = layersList.Add(0, Image(), i, layer.GetName());

		// Set visibility checkbox in column 1
		layersList.SetRowValue(id, 1, layer.IsVisible());

		// Add reference image as child under Annotations layer
		if(layer.GetType() == LAYER_ANNOTATION && !referenceImagePath.IsEmpty()) {
			int childId = layersList.Add(id, Image(), -1, "  " + GetFileName(referenceImagePath));
			layersList.SetRowValue(childId, 1, mapCanvas.GetShowReferenceImage());
			layersList.Open(id);  // Expand the Annotations layer
		}
	}

	// Select active layer
	int activeIndex = layerManager.GetActiveLayerIndex();
	if(activeIndex >= 0 && activeIndex < layerManager.GetLayerCount()) {
		// Find the tree item with this layer index
		for(int i = 0; i < layersList.GetCount(); i++) {
			Value idxVal = layersList.Get(i);  // Get the key value (layer index)
			if(!idxVal.IsVoid() && (int)idxVal == activeIndex) {
				layersList.SetCursor(i);
				break;
			}
		}
	}
}

bool MapEditorApp::Key(dword key, int) {
	switch(key) {
		case K_ESCAPE:
			Break();
			return true;
		case K_CTRL_S:
			SaveFileAction();
			return true;
		case K_CTRL_O:
			OpenFileAction();
			return true;
		case K_CTRL_N:
			NewMapAction();
			return true;
		case K_CTRL_Z:
			UndoAction();
			return true;
		case K_CTRL_Y:
			RedoAction();
			return true;
		case K_F5:
			PlaytestAction();
			return true;

		// Tool selection (UX spec: 1-6 for tools)
		case K_1:  // Draw Wall
			currentTool = TOOL_BRUSH;
			brushTool.SetMode(BRUSH_MODE_PAINT);
			brushTool.SetPaintTile(TILE_WALL);
			mainStatusBar.Set("Tool: Draw Wall");
			return true;
		case K_2:  // Erase Wall
			currentTool = TOOL_ERASER;
			brushTool.SetMode(BRUSH_MODE_ERASE);
			brushTool.SetPaintTile(TILE_WALL);
			mainStatusBar.Set("Tool: Erase Wall");
			return true;
		case K_3:  // Draw Background
			currentTool = TOOL_BRUSH;
			brushTool.SetMode(BRUSH_MODE_PAINT);
			brushTool.SetPaintTile(TILE_BACKGROUND);
			mainStatusBar.Set("Tool: Draw Background");
			return true;
		case K_4:  // Erase Background
			currentTool = TOOL_ERASER;
			brushTool.SetMode(BRUSH_MODE_ERASE);
			brushTool.SetPaintTile(TILE_BACKGROUND);
			mainStatusBar.Set("Tool: Erase Background");
			return true;

		case K_5:  // Enemy Placement
			currentTool = TOOL_ENEMY_PLACEMENT;
			enemyTool.SetMode(PLACEMENT_ADD);
			mainStatusBar.Set("Tool: Add Enemy (P=Patrol, J=Jump, S=Shoot)");
			return true;

		case K_6:  // Droplet Placement
			currentTool = TOOL_DROPLET_PLACEMENT;
			dropletTool.SetMode(PLACEMENT_ADD);
			mainStatusBar.Set("Tool: Add Droplet");
			return true;

		case K_7:  // Pickup Placement
			currentTool = TOOL_PICKUP_PLACEMENT;
			pickupTool.SetMode(PLACEMENT_ADD);
			mainStatusBar.Set("Tool: Add Pickup");
			return true;

		// Legacy shortcuts (kept for backwards compatibility)
		case K_B:  // B for Brush
			currentTool = TOOL_BRUSH;
			brushTool.SetMode(BRUSH_MODE_PAINT);
			mainStatusBar.Set("Tool: Brush");
			return true;
		case K_E:  // E for Eraser
			currentTool = TOOL_ERASER;
			brushTool.SetMode(BRUSH_MODE_ERASE);
			mainStatusBar.Set("Tool: Eraser");
			return true;
		case K_R:  // R for Reference image toggle
			mapCanvas.SetShowReferenceImage(!mapCanvas.GetShowReferenceImage());
			mainStatusBar.Set(mapCanvas.GetShowReferenceImage() ? "Reference image: ON" : "Reference image: OFF");
			return true;
		case K_CTRL_R:  // Ctrl+R to load reference image
			BrowseReferenceImage();
			return true;

		// Brush size shortcuts (UX spec: [ and ] for brush size)
		case K_LBRACKET:  // [ to decrease brush size
			{
				BrushSize currentSize = brushTool.GetBrushSize();
				if(currentSize == BRUSH_2X2) brushTool.SetBrushSize(BRUSH_1X1);
				else if(currentSize == BRUSH_3X3) brushTool.SetBrushSize(BRUSH_2X2);
				else if(currentSize == BRUSH_5X5) brushTool.SetBrushSize(BRUSH_3X3);
				int size = (brushTool.GetBrushSize() == BRUSH_1X1) ? 1 :
				           (brushTool.GetBrushSize() == BRUSH_2X2) ? 2 :
				           (brushTool.GetBrushSize() == BRUSH_3X3) ? 3 : 5;
				mainStatusBar.Set(Format("Brush size: %dx%d", size, size));
			}
			return true;
		case K_RBRACKET:  // ] to increase brush size
			{
				BrushSize currentSize = brushTool.GetBrushSize();
				if(currentSize == BRUSH_1X1) brushTool.SetBrushSize(BRUSH_2X2);
				else if(currentSize == BRUSH_2X2) brushTool.SetBrushSize(BRUSH_3X3);
				else if(currentSize == BRUSH_3X3) brushTool.SetBrushSize(BRUSH_5X5);
				int size = (brushTool.GetBrushSize() == BRUSH_1X1) ? 1 :
				           (brushTool.GetBrushSize() == BRUSH_2X2) ? 2 :
				           (brushTool.GetBrushSize() == BRUSH_3X3) ? 3 : 5;
				mainStatusBar.Set(Format("Brush size: %dx%d", size, size));
			}
			return true;

		// Arrow key panning (UX spec)
		case K_LEFT:
			mapCanvas.PanTo(mapCanvas.GetOffset() + Point(-20, 0));
			return true;
		case K_RIGHT:
			mapCanvas.PanTo(mapCanvas.GetOffset() + Point(20, 0));
			return true;
		case K_UP:
			mapCanvas.PanTo(mapCanvas.GetOffset() + Point(0, -20));
			return true;
		case K_DOWN:
			mapCanvas.PanTo(mapCanvas.GetOffset() + Point(0, 20));
			return true;

		// Tile type shortcuts
		case K_W:  // W for Wall
			brushTool.SetPaintTile(TILE_WALL);
			mainStatusBar.Set("Paint tile: Wall");
			return true;
		case K_G:  // G toggles grid (UX spec)
			mapCanvas.SetShowGrid(!mapCanvas.GetShowGrid());
			mainStatusBar.Set(mapCanvas.GetShowGrid() ? "Grid: ON" : "Grid: OFF");
			return true;
		case K_F:  // F for Fullblock or Fill
			if(GetShift()) {  // Shift+F for Fill tool
				currentTool = TOOL_FILL;
				mainStatusBar.Set("Tool: Fill");
			} else {
				brushTool.SetPaintTile(TILE_FULLBLOCK);
				mainStatusBar.Set("Paint tile: FullBlock");
			}
			return true;

		default:
			break;
	}
	return false;
}

void MapEditorApp::SnapshotActiveLayer() {
	preOpLayerIdx = layerManager.GetActiveLayerIndex();
	if(preOpLayerIdx < 0 || preOpLayerIdx >= layerManager.GetLayerCount()) {
		preOpSnapshot.Clear();
		return;
	}
	const MapGrid& grid = layerManager.GetLayer(preOpLayerIdx).GetGrid();
	int total = grid.GetColumns() * grid.GetRows();
	preOpSnapshot.SetCount(total);
	for(int i = 0; i < total; i++)
		preOpSnapshot[i] = grid.GetTileByIndex(i);
}

void MapEditorApp::CommitUndo(const String& desc) {
	if(preOpLayerIdx < 0 || preOpLayerIdx >= layerManager.GetLayerCount()) return;
	if(preOpSnapshot.IsEmpty()) return;
	const MapGrid& grid = layerManager.GetLayer(preOpLayerIdx).GetGrid();
	int total = min(grid.GetColumns() * grid.GetRows(), preOpSnapshot.GetCount());
	Vector<TileChange> changes;
	for(int i = 0; i < total; i++) {
		TileType newTile = grid.GetTileByIndex(i);
		if(newTile != preOpSnapshot[i]) {
			Point cr = grid.ToColRow(i);
			TileChange ch;
			ch.layerIndex = preOpLayerIdx;
			ch.col = cr.x; ch.row = cr.y;
			ch.oldTile = preOpSnapshot[i]; ch.newTile = newTile;
			changes.Add(ch);
		}
	}
	if(!changes.IsEmpty())
		undoStack.Push(new TileChangeCommand(pick(changes), desc));
	preOpSnapshot.Clear();
}

void MapEditorApp::NewMapAction() {
	if(!PromptYesNo("Create a new empty map? Unsaved changes will be lost."))
		return;
	NewMap();
}

void MapEditorApp::OpenFileAction() {
	FileSel fs;

	// Set initial directory to levels folder - use GetHomeDirFile or proper path resolution
	String exePath = GetExeFilePath();
	String exeDir = GetFileFolder(exePath);
	String levelsDir = AppendFileName(AppendFileName(AppendFileName(exeDir, "share"), "mods/umbrella"), "levels");

	// Try parent directory if not found (when running from bin/)
	if(!DirectoryExists(levelsDir)) {
		levelsDir = AppendFileName(AppendFileName(AppendFileName(GetFileFolder(exeDir), "share"), "mods/umbrella"), "levels");
	}

	if(DirectoryExists(levelsDir)) {
		fs.BaseDir(levelsDir);
	}

	// File types
	fs.Type("JSON Level files", "*.json");
	fs.AllFilesType();

	if(fs.ExecuteOpen("Open Map File")) {
		String selectedFile = fs.Get();

		// If BaseDir was set and the returned path is relative, make it absolute
		if(!IsFullPath(selectedFile) && DirectoryExists(levelsDir)) {
			selectedFile = AppendFileName(levelsDir, selectedFile);
		}

		OpenFile(selectedFile);
	}
}

void MapEditorApp::SaveFileAction() {
	FileSel fs;
	fs.Type("Map files", "*.map");
	if(fs.ExecuteSaveAs("Save Map File")) {
		SaveFile(fs.Get());
	}
}

void MapEditorApp::ExitAction() {
	Break();
}

void MapEditorApp::UndoAction() {
	if(!undoStack.CanUndo()) return;
	undoStack.Undo(layerManager);
	mapCanvas.Refresh();
	mainStatusBar.Set("Undid: " + undoStack.GetRedoDescription());
}

void MapEditorApp::RedoAction() {
	if(!undoStack.CanRedo()) return;
	undoStack.Redo(layerManager);
	mapCanvas.Refresh();
	mainStatusBar.Set("Redid: " + undoStack.GetUndoDescription());
}

void MapEditorApp::ZoomInAction() {
	mapCanvas.SetZoom(1.2);
}

void MapEditorApp::ZoomOutAction() {
	mapCanvas.SetZoom(0.8);
}

void MapEditorApp::ResetZoomAction() {
	mapCanvas.SetZoom(1.0);
}

void MapEditorApp::PlaytestAction() {
	// Generate temporary file path
	String tempPath = GetTempFileName("playtest_level") + ".json";

	// Save current map to temp file
	if(!MapSerializer::SaveToFile(tempPath, layerManager, &enemySpawns, &dropletSpawns, &pickupSpawns)) {
		Exclamation("Failed to save temporary file for playtesting!");
		return;
	}

	LOG("Launching playtest with temp file: " << tempPath);
	mainStatusBar.Set("Launching playtest... Press ESC in game to return.");

	// Create and run GameScreen modally
	GameScreen playtest(tempPath);
	playtest.Run();

	// Clean up temp file
	DeleteFile(tempPath);

	mainStatusBar.Set("Playtest ended. Back to editor.");
	LOG("Playtest ended, temp file removed.");
}

void MapEditorApp::NewMap() {
	int cols = (int)mapColsSpin.GetData();
	int rows = (int)mapRowsSpin.GetData();
	if(cols <= 0) cols = 32;
	if(rows <= 0) rows = 24;
	layerManager.ClearAllLayers();
	layerManager.InitializeDefaultLayers(cols, rows);
	enemySpawns.Clear();
	dropletSpawns.Clear();
	pickupSpawns.Clear();
	currentFilePath = String();
	undoStack.Clear();
	preOpSnapshot.Clear();
	Title("Umbrella Map Editor - New Map");
	RefreshLayersList();
	mapCanvas.Refresh();
	mainStatusBar.Set(Format("New map created (%d\xd7%d)", cols, rows));
}

void MapEditorApp::OpenFile(const String& fileName) {
	LOG("OpenFile: fileName = " << fileName);

	if(MapSerializer::LoadFromFile(fileName, layerManager)) {
		currentFilePath = fileName;
		undoStack.Clear();
		preOpSnapshot.Clear();

		// Update window title
		Title("Umbrella Map Editor - " + GetFileName(fileName));

		// Load spawn points
		enemySpawns.Clear();
		dropletSpawns.Clear();
		pickupSpawns.Clear();
		MapSerializer::LoadEnemySpawns(fileName, enemySpawns);
		MapSerializer::LoadDropletSpawns(fileName, dropletSpawns);
		MapSerializer::LoadPickupSpawns(fileName, pickupSpawns);

		// Auto-load reference image if it exists
		// world1-stage1.json -> map 1-1/frame_000002.jpg
		String baseName = GetFileTitle(fileName);  // "world1-stage1"
		LOG("OpenFile: baseName = " << baseName);

		// Parse worldX-stageY pattern
		int worldNum = 0, stageNum = 0;
		if(baseName.StartsWith("world") && baseName.Find("-stage") >= 0) {
			int dashPos = baseName.Find("-stage");
			LOG("OpenFile: dashPos = " << dashPos);

			worldNum = atoi(baseName.Mid(5, dashPos - 5));
			stageNum = atoi(baseName.Mid(dashPos + 6));
			LOG("OpenFile: parsed world = " << worldNum << ", stage = " << stageNum);

			// Build reference image path: share/mods/umbrella/proprietary/maps/map X-Y/frame_000002.jpg
			String exeDir = GetFileFolder(GetExeFilePath());
			LOG("OpenFile: exeDir = " << exeDir);

			String mapsDir = AppendFileName(AppendFileName(AppendFileName(exeDir, "share"), "mods/umbrella/proprietary"), "maps");
			LOG("OpenFile: trying mapsDir = " << mapsDir << ", exists = " << DirectoryExists(mapsDir));

			// Try parent directory if not found (when running from bin/)
			if(!DirectoryExists(mapsDir)) {
				mapsDir = AppendFileName(AppendFileName(AppendFileName(GetFileFolder(exeDir), "share"), "mods/umbrella/proprietary"), "maps");
				LOG("OpenFile: trying parent mapsDir = " << mapsDir << ", exists = " << DirectoryExists(mapsDir));
			}

			String mapDir = AppendFileName(mapsDir, Format("map %d-%d", worldNum, stageNum));
			LOG("OpenFile: mapDir = " << mapDir << ", exists = " << DirectoryExists(mapDir));

			// Find the first JPG file in the directory
			String refImagePath;
			if(DirectoryExists(mapDir)) {
				FindFile ff(AppendFileName(mapDir, "*.jpg"));
				if(ff) {
					refImagePath = AppendFileName(mapDir, ff.GetName());
					LOG("OpenFile: Found first frame: " << refImagePath);
				}
				else {
					LOG("OpenFile: No JPG files found in " << mapDir);
				}
			}

			if(!refImagePath.IsEmpty() && FileExists(refImagePath)) {
				LOG("OpenFile: Loading reference image from " << refImagePath);
				LoadReferenceImage(refImagePath);
			}
			else {
				LOG("OpenFile: Reference image not found");
			}
		}
		else {
			LOG("OpenFile: Filename pattern doesn't match worldX-stageY");
		}

		// Refresh canvas
		mapCanvas.Refresh();

		// Zoom to fit new map
		mapCanvas.ZoomToFit();

		mainStatusBar.Set("Level loaded: " + GetFileName(fileName));
		AddRecentFile(fileName);
	}
	else {
		LOG("OpenFile: MapSerializer::LoadFromFile failed for " << fileName);
		mainStatusBar.Set("Failed to load level: " + fileName);
	}
}

void MapEditorApp::SaveFile(const String& fileName) {
	if(MapSerializer::SaveToFile(fileName, layerManager, &enemySpawns, &dropletSpawns, &pickupSpawns)) {
		currentFilePath = fileName;
		mainStatusBar.Set("Saved: " + GetFileName(fileName));
		AddRecentFile(fileName);
	}
	else {
		mainStatusBar.Set("Failed to save: " + fileName);
	}
}

void MapEditorApp::LoadReferenceImage(const String& imagePath) {
	LOG("LoadReferenceImage: imagePath = " << imagePath);

	Image img = StreamRaster::LoadFileAny(imagePath);
	if(img.IsEmpty()) {
		LOG("LoadReferenceImage: Failed to load image from " << imagePath);
		Exclamation("Failed to load image: " + imagePath);
		return;
	}

	LOG("LoadReferenceImage: Successfully loaded image, size = " << img.GetSize());

	// Fix RGB/BGR swap - swap R and B channels
	ImageBuffer ib(img);
	RGBA* pixels = ib.Begin();
	int count = ib.GetLength();
	for(int i = 0; i < count; i++) {
		byte temp = pixels[i].r;
		pixels[i].r = pixels[i].b;
		pixels[i].b = temp;
	}
	img = ib;

	referenceImagePath = imagePath;
	mapCanvas.SetReferenceImage(img);
	mapCanvas.SetShowReferenceImage(true);
	mainStatusBar.Set("Reference image loaded: " + GetFileName(imagePath));

	// Refresh layers list to show reference image as child of Annotations layer
	RefreshLayersList();
}

void MapEditorApp::BrowseReferenceImage() {
	FileSel fs;

	// Set initial directory to proprietary maps
	String exeDir = GetFileFolder(GetExeFilePath());
	String mapsDir = AppendFileName(AppendFileName(AppendFileName(exeDir, "share"), "mods/umbrella/proprietary"), "maps");

	// Try parent directory if not found (when running from bin/)
	if(!DirectoryExists(mapsDir)) {
		mapsDir = AppendFileName(AppendFileName(AppendFileName(GetFileFolder(exeDir), "share"), "mods/umbrella/proprietary"), "maps");
	}

	if(DirectoryExists(mapsDir)) {
		fs.BaseDir(mapsDir);
	}

	// File types
	fs.Type("Image files", "*.jpg *.jpeg *.png");
	fs.AllFilesType();

	if(fs.ExecuteOpen("Open Reference Image")) {
		LoadReferenceImage(fs.Get());
	}
}

void MapEditorApp::SetupPropertiesPanel() {
	propertiesPanel.SetFrame(InsetFrame());

	int yPos = 10;

	// Map Size section
	mapSizeLabel.SetText("MAP SIZE");
	mapSizeLabel.SetAlign(ALIGN_CENTER);
	propertiesPanel.Add(mapSizeLabel.HSizePos(10, 10).TopPos(yPos, 20));
	yPos += 30;

	// Map columns
	mapColsLabel.SetText("Columns:");
	propertiesPanel.Add(mapColsLabel.LeftPos(10, 80).TopPos(yPos, 20));
	mapColsSpin.MinMax(1, 200);
	mapColsSpin.SetData(32);  // Default
	propertiesPanel.Add(mapColsSpin.LeftPos(100, 80).TopPos(yPos, 20));
	yPos += 30;

	// Map rows
	mapRowsLabel.SetText("Rows:");
	propertiesPanel.Add(mapRowsLabel.LeftPos(10, 80).TopPos(yPos, 20));
	mapRowsSpin.MinMax(1, 200);
	mapRowsSpin.SetData(24);  // Default
	propertiesPanel.Add(mapRowsSpin.LeftPos(100, 80).TopPos(yPos, 20));
	yPos += 30;

	// Apply button
	applyMapSizeBtn.SetLabel("Apply");
	applyMapSizeBtn <<= callback(this, &MapEditorApp::ApplyMapSizeAction);
	propertiesPanel.Add(applyMapSizeBtn.LeftPos(10, 80).TopPos(yPos, 25));
	yPos += 35;

	// Initialize with current map size if layers exist
	if(layerManager.GetLayerCount() > 0) {
		const MapGrid& grid = layerManager.GetLayer(0).GetGrid();
		mapColsSpin.SetData(grid.GetMapCols());
		mapRowsSpin.SetData(grid.GetMapRows());
	}
}

void MapEditorApp::ApplyMapSizeAction() {
	int newCols = mapColsSpin.GetData();
	int newRows = mapRowsSpin.GetData();

	// Update all layers
	for(int i = 0; i < layerManager.GetLayerCount(); i++) {
		layerManager.GetLayer(i).GetGrid().SetMapArea(newCols, newRows);
	}

	mapCanvas.Refresh();
	mainStatusBar.Set(Format("Map size updated to %d x %d", newCols, newRows));
}

void MapEditorApp::LoadRecentFiles() {
	// Load from config file in ~/.config/Umbrella/MapEditor.recent
	String configDir = GetHomeDirFile(".config/Umbrella");
	String recentPath = AppendFileName(configDir, "MapEditor.recent");

	if(FileExists(recentPath)) {
		String content = LoadFile(recentPath);
		Vector<String> lines = Split(content, '\n');
		for(const String& line : lines) {
			String trimmed = TrimBoth(line);
			if(!trimmed.IsEmpty() && FileExists(trimmed)) {
				recentFiles.Add(trimmed);
			}
		}
		// Limit to 10 most recent
		while(recentFiles.GetCount() > 10) {
			recentFiles.Remove(recentFiles.GetCount() - 1);
		}
	}
}

void MapEditorApp::SaveRecentFiles() {
	// Save to config file in ~/.config/Umbrella/MapEditor.recent
	String configDir = GetHomeDirFile(".config/Umbrella");
	RealizeDirectory(configDir);

	String recentPath = AppendFileName(configDir, "MapEditor.recent");
	String content;
	for(const String& file : recentFiles) {
		content << file << "\n";
	}
	::SaveFile(recentPath, content);
}

void MapEditorApp::AddRecentFile(const String& filePath) {
	// Remove if already exists
	int idx = FindIndex(recentFiles, filePath);
	if(idx >= 0) {
		recentFiles.Remove(idx);
	}

	// Add to front
	recentFiles.Insert(0, filePath);

	// Limit to 10 most recent
	while(recentFiles.GetCount() > 10) {
		recentFiles.Remove(recentFiles.GetCount() - 1);
	}

	SaveRecentFiles();
}

void MapEditorApp::OpenRecentFile(int index) {
	if(index >= 0 && index < recentFiles.GetCount()) {
		String filePath = recentFiles[index];
		if(FileExists(filePath)) {
			OpenFile(filePath);
		}
		else {
			Exclamation("File not found: " + filePath);
			// Remove from recent files
			recentFiles.Remove(index);
			SaveRecentFiles();
		}
	}
}

// MapEditorApp is now available for use from main.cpp
// The GUI_APP_MAIN is in main.cpp, not here
