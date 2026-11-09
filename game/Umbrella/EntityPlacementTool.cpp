#include "Umbrella.h"
#include "EntityPlacementTool.h"

using namespace Upp;

// ============================================================================
// EnemyPlacementTool
// ============================================================================

EnemyPlacementTool::EnemyPlacementTool()
	: EntityPlacementTool(ENTITY_ENEMY)
{
	enemySpawns = nullptr;
	selectedType = ENEMY_PATROLLER;
	selectedFacing = 1;
}

void EnemyPlacementTool::Click(int col, int row) {
	if(!enemySpawns) return;

	if(mode == PLACEMENT_REMOVE) {
		// Remove spawn at this position
		int idx = FindSpawnAt(col, row);
		if(idx >= 0) {
			enemySpawns->Remove(idx);
		}
	}
	else {
		// Check if spawn already exists at this position
		int existing = FindSpawnAt(col, row);
		if(existing >= 0) {
			// Update existing spawn
			(*enemySpawns)[existing].type = selectedType;
			(*enemySpawns)[existing].facing = selectedFacing;
		}
		else {
			// Add new spawn
			EnemySpawnPoint& spawn = enemySpawns->Add();
			spawn.col = col;
			spawn.row = row;
			spawn.type = selectedType;
			spawn.facing = selectedFacing;
		}
	}
}

void EnemyPlacementTool::Render(Draw& w, int col, int row, Point offset, double zoom, int gridSize) {
	if(!enemySpawns) return;

	// Render all enemy spawn points
	for(int i = 0; i < enemySpawns->GetCount(); i++) {
		const EnemySpawnPoint& spawn = (*enemySpawns)[i];

		// Calculate screen position
		int screenX = (int)((spawn.col * gridSize + offset.x) * zoom);
		int screenY = (int)((spawn.row * gridSize + offset.y) * zoom);
		int size = (int)(gridSize * zoom);

		// Choose color based on enemy type
		Color markerColor;
		String typeLabel;
		switch(spawn.type) {
			case ENEMY_PATROLLER:
				markerColor = Color(255, 100, 100);  // Red
				typeLabel = "P";
				break;
			case ENEMY_JUMPER:
				markerColor = Color(100, 255, 100);  // Green
				typeLabel = "J";
				break;
			case ENEMY_SHOOTER:
				markerColor = Color(100, 100, 255);  // Blue
				typeLabel = "S";
				break;
		case ENEMY_FLYER:
				markerColor = Color(180, 0, 220);  // Purple
				typeLabel = "F";
				break;
		}

		// Draw filled circle with black border (drawn manually with lines)
		w.DrawEllipse(screenX, screenY, size, size, markerColor);

		// Draw type label
		Font font = Arial(max(10, (int)(12 * zoom))).Bold();
		Size textSize = GetTextSize(typeLabel, font);
		w.DrawText(screenX + size/2 - textSize.cx/2,
		           screenY + size/2 - textSize.cy/2,
		           typeLabel, font, White());

		// Draw facing indicator (arrow)
		int arrowSize = size / 4;
		int arrowX = screenX + size/2 + (spawn.facing > 0 ? size/3 : -size/3);
		int arrowY = screenY + size/2;
		if(spawn.facing > 0) {
			// Right arrow
			w.DrawLine(arrowX - arrowSize, arrowY - arrowSize, arrowX, arrowY, 2, White());
			w.DrawLine(arrowX - arrowSize, arrowY + arrowSize, arrowX, arrowY, 2, White());
		}
		else {
			// Left arrow
			w.DrawLine(arrowX + arrowSize, arrowY - arrowSize, arrowX, arrowY, 2, White());
			w.DrawLine(arrowX + arrowSize, arrowY + arrowSize, arrowX, arrowY, 2, White());
		}
	}

	// Draw preview at cursor position (only in ADD mode)
	if(mode == PLACEMENT_ADD && col >= 0 && row >= 0) {
		int screenX = (int)((col * gridSize + offset.x) * zoom);
		int screenY = (int)((row * gridSize + offset.y) * zoom);
		int size = (int)(gridSize * zoom);

		// Semi-transparent preview (lighter colors for preview)
		Color previewColor;
		String typeLabel;
		switch(selectedType) {
			case ENEMY_PATROLLER:
				previewColor = Color(255, 150, 150);  // Light red
				typeLabel = "P";
				break;
			case ENEMY_JUMPER:
				previewColor = Color(150, 255, 150);  // Light green
				typeLabel = "J";
				break;
			case ENEMY_SHOOTER:
				previewColor = Color(150, 150, 255);  // Light blue
				typeLabel = "S";
				break;
		case ENEMY_FLYER:
				previewColor = Color(200, 100, 255);  // Light purple
				typeLabel = "F";
				break;
		}

		w.DrawEllipse(screenX, screenY, size, size, previewColor);

		Font font = Arial(max(10, (int)(12 * zoom))).Bold();
		Size textSize = GetTextSize(typeLabel, font);
		w.DrawText(screenX + size/2 - textSize.cx/2,
		           screenY + size/2 - textSize.cy/2,
		           typeLabel, font, White());
	}
}

int EnemyPlacementTool::FindSpawnAt(int col, int row) const {
	if(!enemySpawns) return -1;

	for(int i = 0; i < enemySpawns->GetCount(); i++) {
		if((*enemySpawns)[i].col == col && (*enemySpawns)[i].row == row) {
			return i;
		}
	}
	return -1;
}

// ============================================================================
// DropletPlacementTool
// ============================================================================

DropletPlacementTool::DropletPlacementTool()
	: EntityPlacementTool(ENTITY_DROPLET)
{
	dropletSpawns = nullptr;
	selectedType = DROPLET_RAINBOW;
	selectedDirection = 0;
	selectedInterval = 2000;
}

void DropletPlacementTool::Click(int col, int row) {
	if(!dropletSpawns) return;

	if(mode == PLACEMENT_REMOVE) {
		// Remove spawn at this position
		int idx = FindSpawnAt(col, row);
		if(idx >= 0) {
			dropletSpawns->Remove(idx);
		}
	}
	else {
		// Check if spawn already exists at this position
		int existing = FindSpawnAt(col, row);
		if(existing >= 0) {
			// Update existing spawn
			(*dropletSpawns)[existing].mode = selectedType;
			(*dropletSpawns)[existing].direction = selectedDirection;
			(*dropletSpawns)[existing].intervalMs = selectedInterval;
		}
		else {
			// Add new spawn
			DropletSpawnPoint& spawn = dropletSpawns->Add();
			spawn.col = col;
			spawn.row = row;
			spawn.mode = selectedType;
			spawn.direction = selectedDirection;
			spawn.intervalMs = selectedInterval;
			spawn.enabled = true;
			spawn.timer = 0.0f;
		}
	}
}

void DropletPlacementTool::Render(Draw& w, int col, int row, Point offset, double zoom, int gridSize) {
	if(!dropletSpawns) return;

	// Render all droplet spawn points
	for(int i = 0; i < dropletSpawns->GetCount(); i++) {
		const DropletSpawnPoint& spawn = (*dropletSpawns)[i];

		// Calculate screen position
		int screenX = (int)((spawn.col * gridSize + offset.x) * zoom);
		int screenY = (int)((spawn.row * gridSize + offset.y) * zoom);
		int size = (int)(gridSize * zoom * 0.6);  // Smaller than enemies

		// Choose color based on droplet type
		Color markerColor;
		switch(spawn.mode) {
			case DROPLET_RAINBOW:
				markerColor = Color(255, 200, 100);  // Yellow/orange
				break;
			case DROPLET_ICE:
				markerColor = Color(100, 200, 255);  // Cyan
				break;
			case DROPLET_FIRE:
				markerColor = Color(255, 100, 50);  // Orange/red
				break;
		}

		// Draw filled circle (droplet shape)
		w.DrawEllipse(screenX + size/4, screenY, size, size, markerColor);

		// Draw interval indicator (small text at bottom)
		if(spawn.intervalMs > 0) {
			String intervalText = Format("%d", spawn.intervalMs / 1000) + "s";
			Font smallFont = Arial(max(8, (int)(10 * zoom)));
			Size textSize = GetTextSize(intervalText, smallFont);
			w.DrawText(screenX + size/2 - textSize.cx/2,
			           screenY + size + 2,
			           intervalText, smallFont, Black());
		}
	}

	// Draw preview at cursor position (only in ADD mode)
	if(mode == PLACEMENT_ADD && col >= 0 && row >= 0) {
		int screenX = (int)((col * gridSize + offset.x) * zoom);
		int screenY = (int)((row * gridSize + offset.y) * zoom);
		int size = (int)(gridSize * zoom * 0.6);

		// Preview (lighter colors)
		Color previewColor;
		switch(selectedType) {
			case DROPLET_RAINBOW:
				previewColor = Color(255, 220, 150);  // Light yellow/orange
				break;
			case DROPLET_ICE:
				previewColor = Color(150, 220, 255);  // Light cyan
				break;
			case DROPLET_FIRE:
				previewColor = Color(255, 150, 100);  // Light orange
				break;
		}

		w.DrawEllipse(screenX + size/4, screenY, size, size, previewColor);
	}
}

int DropletPlacementTool::FindSpawnAt(int col, int row) const {
	if(!dropletSpawns) return -1;

	for(int i = 0; i < dropletSpawns->GetCount(); i++) {
		if((*dropletSpawns)[i].col == col && (*dropletSpawns)[i].row == row) {
			return i;
		}
	}
	return -1;
}

// ============================================================================
// PickupPlacementTool
// ============================================================================

PickupPlacementTool::PickupPlacementTool()
	: EntityPlacementTool(ENTITY_PICKUP)
{
	pickupSpawns = nullptr;
	selectedType = PU_HEART;
}

void PickupPlacementTool::Click(int col, int row) {
	if(!pickupSpawns) return;

	if(mode == PLACEMENT_REMOVE) {
		int idx = FindSpawnAt(col, row);
		if(idx >= 0) {
			pickupSpawns->Remove(idx);
		}
	}
	else {
		int existing = FindSpawnAt(col, row);
		if(existing >= 0) {
			(*pickupSpawns)[existing].type = selectedType;
		}
		else {
			PickupSpawnPoint& spawn = pickupSpawns->Add();
			spawn.col = col;
			spawn.row = row;
			spawn.type = selectedType;
		}
	}
}

void PickupPlacementTool::Render(Draw& w, int col, int row, Point offset, double zoom, int gridSize) {
	if(!pickupSpawns) return;

	// Render all pickup spawn points
	for(int i = 0; i < pickupSpawns->GetCount(); i++) {
		const PickupSpawnPoint& spawn = (*pickupSpawns)[i];

		int screenX = (int)((spawn.col * gridSize + offset.x) * zoom);
		int screenY = (int)((spawn.row * gridSize + offset.y) * zoom);
		int size = (int)(gridSize * zoom * 0.8);

		Color markerColor;
		String typeLabel;
		switch(spawn.type) {
			case PU_HEART:     markerColor = Color(255, 80, 120);  typeLabel = "H"; break;
			case PU_GEM:       markerColor = Color(80, 200, 255);  typeLabel = "G"; break;
			case PU_LIGHTNING: markerColor = Color(255, 255, 80);  typeLabel = "L"; break;
			case PU_SPEED:     markerColor = Color(80, 255, 120);  typeLabel = "S"; break;
		}

		// Draw diamond shape (rotated square)
		int cx = screenX + size / 2;
		int cy = screenY + size / 2;
		int r = size / 2;
		Vector<Point> pts;
		pts.Add(Point(cx, cy - r));
		pts.Add(Point(cx + r, cy));
		pts.Add(Point(cx, cy + r));
		pts.Add(Point(cx - r, cy));
		w.DrawPolygon(pts, markerColor, 1, Black());

		// Draw type label
		Font font = Arial(max(8, (int)(10 * zoom))).Bold();
		Size textSize = GetTextSize(typeLabel, font);
		w.DrawText(cx - textSize.cx / 2, cy - textSize.cy / 2,
		           typeLabel, font, Black());
	}

	// Draw preview at cursor position (only in ADD mode)
	if(mode == PLACEMENT_ADD && col >= 0 && row >= 0) {
		int screenX = (int)((col * gridSize + offset.x) * zoom);
		int screenY = (int)((row * gridSize + offset.y) * zoom);
		int size = (int)(gridSize * zoom * 0.8);

		Color previewColor;
		String typeLabel;
		switch(selectedType) {
			case PU_HEART:     previewColor = Color(255, 140, 170); typeLabel = "H"; break;
			case PU_GEM:       previewColor = Color(140, 220, 255); typeLabel = "G"; break;
			case PU_LIGHTNING: previewColor = Color(255, 255, 140); typeLabel = "L"; break;
			case PU_SPEED:     previewColor = Color(140, 255, 170); typeLabel = "S"; break;
		}

		int cx = screenX + size / 2;
		int cy = screenY + size / 2;
		int r = size / 2;
		Vector<Point> pts;
		pts.Add(Point(cx, cy - r));
		pts.Add(Point(cx + r, cy));
		pts.Add(Point(cx, cy + r));
		pts.Add(Point(cx - r, cy));
		w.DrawPolygon(pts, previewColor, 1, Black());

		Font font = Arial(max(8, (int)(10 * zoom))).Bold();
		Size textSize = GetTextSize(typeLabel, font);
		w.DrawText(cx - textSize.cx / 2, cy - textSize.cy / 2,
		           typeLabel, font, Black());
	}
}

int PickupPlacementTool::FindSpawnAt(int col, int row) const {
	if(!pickupSpawns) return -1;

	for(int i = 0; i < pickupSpawns->GetCount(); i++) {
		if((*pickupSpawns)[i].col == col && (*pickupSpawns)[i].row == row) {
			return i;
		}
	}
	return -1;
}
