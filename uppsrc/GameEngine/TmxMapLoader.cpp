#include "TmxMapLoader.h"
#include <plugin/png/Png.h>  // For loading tileset images
#include <plugin/z/Z.h>      // For compression support

NAMESPACE_UPP

TmxMap::TmxMap() {
}

const Tile* TmxMap::GetTileAt(int layerIndex, int x, int y) const {
    if (layerIndex < 0 || layerIndex >= layers.GetCount()) {
        return nullptr;
    }
    
    const MapLayer& layer = layers[layerIndex];
    if (x < 0 || x >= layer.width || y < 0 || y >= layer.height) {
        return nullptr;
    }
    
    int index = y * layer.width + x;
    if (index >= layer.tiles.GetCount()) {
        return nullptr;
    }
    
    return &layer.tiles[index];
}

int TmxMap::GetGlobalTileId(int localTileId, int tilesetIndex) const {
    if (tilesetIndex < 0 || tilesetIndex >= tilesets.GetCount()) {
        return -1;
    }
    
    const Tileset& tileset = tilesets[tilesetIndex];
    return tileset.firstGid + localTileId;
}

TmxMapLoader::TmxMapLoader() {
}

std::shared_ptr<TmxMap> TmxMapLoader::LoadMap(const String& filepath) {
    auto map = std::make_shared<TmxMap>();
    
    // Read the TMX file
    String tmxContent = LoadFile(filepath);
    if (tmxContent.IsEmpty()) {
        LOG("Failed to load TMX file: " << filepath);
        return nullptr;
    }
    
    if (!ParseXML(tmxContent, map)) {
        LOG("Failed to parse TMX file: " << filepath);
        return nullptr;
    }
    
    return map;
}

std::shared_ptr<TmxMap> TmxMapLoader::LoadMapFromVFS(const String& vfsPath, std::shared_ptr<VFS> vfs) {
    if (!vfs) return nullptr;
    
    auto map = std::make_shared<TmxMap>();
    
    // Read file from VFS
    String tmxContent = vfs->LoadString(vfsPath);
    if (tmxContent.IsEmpty()) {
        LOG("Failed to load TMX file from VFS: " << vfsPath);
        return nullptr;
    }
    
    if (!ParseXML(tmxContent, map)) {
        LOG("Failed to parse TMX file from VFS: " << vfsPath);
        return nullptr;
    }
    
    return map;
}

bool TmxMapLoader::ParseXML(const String& xmlContent, std::shared_ptr<TmxMap> map) {
    if (!map) return false;
    
    XmlParser parser(xmlContent);
    
    try {
        // Parse the root map element
        if (!parser.IsTag("map")) {
            LOG("TMX file doesn't start with 'map' tag");
            return false;
        }
        
        // Get map attributes
        map->orientation = parser.ReadAttr("orientation", "orthogonal");
        map->renderOrder = parser.ReadAttr("renderorder", "right-down");
        map->width = StrInt(parser.ReadAttr("width", "0"));
        map->height = StrInt(parser.ReadAttr("height", "0"));
        map->tileWidth = StrInt(parser.ReadAttr("tilewidth", "0"));
        map->tileHeight = StrInt(parser.ReadAttr("tileheight", "0"));
        
        // Parse elements inside the map
        while (!parser.IsEof()) {
            if (parser.IsTag("tileset")) {
                Tileset tileset = ParseTileset(parser);
                map->AddTileset(tileset);
            }
            else if (parser.IsTag("layer")) {
                MapLayer layer = ParseLayer(parser, map->width, map->height);
                map->AddLayer(layer);
            }
            else if (parser.IsTag("objectgroup")) {
                ObjectGroup group = ParseObjectGroup(parser);
                map->AddObjectGroup(group);
            }
            else if (parser.IsTag("properties") || parser.IsTag("property")) {
                // Skip properties for now
                parser.SkipCurrent();
            }
            else {
                parser.PassToElement();
            }
        }
    }
    catch (XmlError& e) {
        LOG("XML parsing error: " << e);
        return false;
    }
    catch (...) {
        LOG("Unknown error while parsing TMX file");
        return false;
    }
    
    return true;
}

Tileset TmxMapLoader::ParseTileset(XmlParser& parser) {
    Tileset tileset;
    
    // Get tileset attributes
    tileset.name = parser.ReadAttr("name", "");
    tileset.firstGid = StrInt(parser.ReadAttr("firstgid", "1"));
    tileset.tileWidth = StrInt(parser.ReadAttr("tilewidth", "0"));
    tileset.tileHeight = StrInt(parser.ReadAttr("tileheight", "0"));
    tileset.spacing = StrInt(parser.ReadAttr("spacing", "0"));
    tileset.margin = StrInt(parser.ReadAttr("margin", "0"));
    
    // Parse tileset contents
    while (!parser.IsEof() && !parser.IsEndTag("tileset")) {
        if (parser.IsTag("image")) {
            String sourcePath = parser.ReadAttr("source", "");
            
            // Load the tileset image
            if (!sourcePath.IsEmpty()) {
                String imagePath = AppendFileName(GetFileFolder(parser.GetSource()), sourcePath);
                tileset.image = LoadImageFile(imagePath);
            }
            
            parser.SkipCurrent();
        }
        else if (parser.IsTag("tile")) {
            // Parse individual tile properties if needed
            parser.SkipCurrent();
        }
        else {
            parser.PassToElement();
        }
    }
    
    return tileset;
}

MapLayer TmxMapLoader::ParseLayer(XmlParser& parser, int mapWidth, int mapHeight) {
    MapLayer layer;
    
    // Get layer attributes
    layer.name = parser.ReadAttr("name", "");
    layer.width = StrInt(parser.ReadAttr("width", AsString(mapWidth)));
    layer.height = StrInt(parser.ReadAttr("height", AsString(mapHeight)));
    layer.opacity = StrDouble(parser.ReadAttr("opacity", "1.0"));
    String visibleStr = parser.ReadAttr("visible", "1");
    layer.visible = (visibleStr == "1" || visibleStr == "true");
    layer.offset.x = StrDouble(parser.ReadAttr("offsetx", "0"));
    layer.offset.y = StrDouble(parser.ReadAttr("offsety", "0"));
    
    // Default initialize tiles
    layer.tiles.SetCount(layer.width * layer.height);
    for (int i = 0; i < layer.tiles.GetCount(); i++) {
        layer.tiles[i].id = -1; // No tile
    }
    
    // Parse layer contents
    while (!parser.IsEof() && !parser.IsEndTag("layer")) {
        if (parser.IsTag("data")) {
            String encoding = parser.ReadAttr("encoding", "");
            String compression = parser.ReadAttr("compression", "");
            
            String dataContent = parser.ReadString();
            
            layer.tiles = DecodeTileData(dataContent, encoding, compression, 
                                       layer.width, layer.height);
        }
        else if (parser.IsTag("properties") || parser.IsTag("property")) {
            // Skip properties for now
            parser.SkipCurrent();
        }
        else {
            parser.PassToElement();
        }
    }
    
    return layer;
}

ObjectGroup TmxMapLoader::ParseObjectGroup(XmlParser& parser) {
    ObjectGroup group;
    
    // Get object group attributes
    group.name = parser.ReadAttr("name", "");
    String colorStr = parser.ReadAttr("color", "");
    if (!colorStr.IsEmpty() && colorStr.GetCount() >= 7 && colorStr[0] == '#') {
        // Parse hex color
        group.color = Color(
            StrInt("0x" + colorStr.Mid(1, 2), 0),
            StrInt("0x" + colorStr.Mid(3, 2), 0),
            StrInt("0x" + colorStr.Mid(5, 2), 0)
        );
    } else {
        group.color = Color(255, 255, 255); // Default white
    }
    group.visible = (parser.ReadAttr("visible", "1") == "1");
    group.opacity = StrDouble(parser.ReadAttr("opacity", "1.0"));
    group.offset.x = StrDouble(parser.ReadAttr("offsetx", "0"));
    group.offset.y = StrDouble(parser.ReadAttr("offsety", "0"));
    
    // Parse objects in the group
    while (!parser.IsEof() && !parser.IsEndTag("objectgroup")) {
        if (parser.IsTag("object")) {
            // For now, we'll just store the raw object data as a value
            // In a real implementation, we'd parse specific object properties
            Value objValue;
            // Extract object properties: id, name, type, x, y, width, height, etc.
            String idStr = parser.ReadAttr("id", "");
            String name = parser.ReadAttr("name", "");
            String type = parser.ReadAttr("type", "");
            String xStr = parser.ReadAttr("x", "0");
            String yStr = parser.ReadAttr("y", "0");
            String widthStr = parser.ReadAttr("width", "0");
            String heightStr = parser.ReadAttr("height", "0");
            
            // Store the object properties in the value
            objValue << idStr << name << type << xStr << yStr << widthStr << heightStr;
            
            group.objects.Add(objValue);
            
            // Skip the object content for now (might contain polyline, polygon, etc.)
            parser.SkipCurrent();
        }
        else if (parser.IsTag("properties") || parser.IsTag("property")) {
            // Skip properties for now
            parser.SkipCurrent();
        }
        else {
            parser.PassToElement();
        }
    }
    
    return group;
}

Vector<Tile> TmxMapLoader::DecodeTileData(const String& data, const String& encoding, 
                                         const String& compression, int width, int height) {
    Vector<Tile> tiles;
    tiles.SetCount(width * height);
    
    if (encoding == "csv") {
        // Parse comma-separated values
        Vector<String> values = Split(data, ',');
        for (int i = 0; i < min(values.GetCount(), tiles.GetCount()); i++) {
            int gid = StrInt(values[i]);
            
            tiles[i].gid = gid;
            tiles[i].id = gid; // Simplified: using GID as ID
            
            // Extract flipping flags from GID (bits 31, 30, 29)
            if (gid & (1 << 31)) tiles[i].flippedHorizontally = true;
            if (gid & (1 << 30)) tiles[i].flippedVertically = true;
            if (gid & (1 << 29)) tiles[i].flippedDiagonally = true;
            
            // Remove flipping flags from the actual tile ID
            tiles[i].id = gid & 0x1FFFFFFF; // Clear the 3 high bits
        }
    }
    else if (encoding == "base64") {
        // Decode base64 data
        String decoded = DecodeBase64(data);
        
        if (compression == "gzip") {
            try {
                ZDecompress zdec;
                decoded = zdec(decoded);
            }
            catch (...) {
                LOG("Failed to decompress gzip data");
                // Continue with original data
            }
        }
        else if (compression == "zlib") {
            try {
                ZLibDecompress zdec;
                decoded = zdec(decoded);
            }
            catch (...) {
                LOG("Failed to decompress zlib data");
                // Continue with original data
            }
        }
        
        // Parse as 32-bit integers (little-endian)
        int numInts = decoded.GetCount() / 4;
        tiles.SetCount(min(numInts, width * height));
        
        for (int i = 0; i < tiles.GetCount(); i++) {
            // Read 32-bit integer in little-endian format
            int gid = (unsigned char)decoded[i*4] |
                     ((unsigned char)decoded[i*4+1] << 8) |
                     ((unsigned char)decoded[i*4+2] << 16) |
                     ((unsigned char)decoded[i*4+3] << 24);
            
            tiles[i].gid = gid;
            tiles[i].id = gid; // Simplified: using GID as ID
            
            // Extract flipping flags from GID (bits 31, 30, 29)
            if (gid & (1 << 31)) tiles[i].flippedHorizontally = true;
            if (gid & (1 << 30)) tiles[i].flippedVertically = true;
            if (gid & (1 << 29)) tiles[i].flippedDiagonally = true;
            
            // Remove flipping flags from the actual tile ID
            tiles[i].id = gid & 0x1FFFFFFF; // Clear the 3 high bits
        }
    }
    else {
        // Raw format or unknown format
        LOG("Unknown encoding format: " << encoding);
    }
    
    return tiles;
}

END_UPP_NAMESPACE