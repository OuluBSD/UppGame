# Advanced Collections

## Overview

The Advanced Collections in GameEngine provide libgdx-style performance-optimized collection types specifically designed for game development. These collections address common performance bottlenecks in games by providing more efficient alternatives to standard containers.

## Key Features

- **ObjectSet**: Optimized set implementation for fast iteration over objects
- **IntMap**: Specialized map from int to value type with better performance than generic maps
- **IntFloatMap**: Optimized map from int to float for common game scenarios
- **IntIntMap**: Optimized map from int to int
- **ObjectMap**: Optimized map from objects to objects
- **Performance Optimized**: All collections are optimized for game development scenarios
- **libgdx Compatible**: Follows libgdx patterns for familiarity

## Basic Usage

### ObjectSet - Optimized object set with fast iteration

```cpp
#include <GameEngine/GameEngine.h>

// Create an ObjectSet for strings
ObjectSet<String> objectSet;

// Add elements
objectSet.Add("entity1");
objectSet.Add("entity2");
objectSet.Add("entity3");

// Check for existence
if (objectSet.Contains("entity1")) {
    // Element exists
}

// Fast iteration (faster than standard hash set iteration)
for (const auto& item : objectSet) {
    // Process each item efficiently
    std::cout << item << std::endl;
}

// Remove elements
objectSet.Remove("entity2");

// Get size
int count = objectSet.GetCount();
```

### IntMap - Optimized map from int to value

```cpp
// Create an IntMap from int to String
IntMap<String> intMap;

// Add key-value pairs
intMap.Put(1, "first");
intMap.Put(2, "second");
intMap.Put(3, "third");

// Retrieve values
String value = intMap.Get(2);  // Returns "second"
String defaultValue = intMap.Get(99, "not found");  // Returns "not found"

// Check for keys
if (intMap.Contains(1)) {
    // Key exists
}

// Iteration
for (const auto& pair : intMap) {
    std::cout << "Key: " << pair.key << ", Value: " << pair.value << std::endl;
}

// Get all keys or values
Vector<int> keys = intMap.GetKeys();
Vector<String> values = intMap.GetValues();
```

### IntFloatMap - Optimized map from int to float

```cpp
// Create an IntFloatMap (optimized for int->float mappings)
IntFloatMap floatMap;

// Add values
floatMap.Put(100, 3.14);
floatMap.Put(101, 2.71);
floatMap.Put(102, 1.41);

// Retrieve values
double value = floatMap.Get(100);  // Returns 3.14
double defaultVal = floatMap.Get(999, -1.0);  // Returns -1.0 for non-existent key

// Check existence
if (floatMap.Contains(101)) {
    // Key exists
}
```

### IntIntMap - Optimized map from int to int

```cpp
// Create an IntIntMap
IntIntMap intIntMap;

// Add values
intIntMap.Put(1, 100);
intIntMap.Put(2, 200);
intIntMap.Put(3, 300);

// Retrieve values
int value = intIntMap.Get(2);  // Returns 200
int defaultVal = intIntMap.Get(99, -1);  // Returns -1 for non-existent key
```

### ObjectMap - Optimized map from object to object

```cpp
// Create an ObjectMap from String to int
ObjectMap<String, int> objMap;

// Add key-value pairs
objMap.Put("health", 100);
objMap.Put("mana", 50);
objMap.Put("level", 5);

// Retrieve values
int health = objMap.Get("health");  // Returns 100
int defaultVal = objMap.Get("experience", 0);  // Returns 0 for non-existent key

// Check existence
if (objMap.Contains("mana")) {
    // Key exists
}
```

## Advanced Usage

### Using Collections with Game Entities

```cpp
struct Entity {
    int id;
    String type;
    Point3 position;
    
    Entity(int i, const String& t, const Point3& p) : id(i), type(t), position(p) {}
};

// Create a set of active entities
ObjectSet<Entity> activeEntities;

// Create maps for fast lookups
IntMap<Entity> entityById;        // Map entity ID to entity
IntMap<Point3> positionById;      // Map entity ID to position
IntFloatMap healthById;           // Map entity ID to health value

// Add an entity
Entity player(1, "Player", Point3(0, 0, 0));
activeEntities.Add(player);
entityById.Put(player.id, player);
positionById.Put(player.id, player.position);
healthById.Put(player.id, 100.0); // Full health
```

### Performance-Optimized Game Loop Pattern

```cpp
// Efficiently process all active entities using ObjectSet
void ProcessEntities() {
    // Fast iteration over all active entities
    for (const auto& entity : activeEntities) {
        // Process each entity efficiently
        UpdateEntity(entity);
    }
}

// Efficient lookups during processing
void UpdateEntity(const Entity& entity) {
    int id = entity.id;
    
    // Quick lookups using specialized maps
    Point3 pos = positionById.Get(id);
    double health = healthById.Get(id);
    
    // Update position, check health, etc.
}
```

### Memory-Efficient Component Storage

```cpp
// Using specialized maps to store component data efficiently
IntMap<TransformComponent> transformComponents;
IntMap<PhysicsComponent> physicsComponents;
IntMap<RenderComponent> renderComponents;

// Fast retrieval of specific components for an entity
TransformComponent GetTransform(int entityId) {
    return transformComponents.Get(entityId);
}

// Batch operations using GetKeys() for better cache performance
void UpdateAllTransforms() {
    Vector<int> entityIds = transformComponents.GetKeys();
    for (int id : entityIds) {
        TransformComponent& transform = transformComponents.GetPtr(id);
        // Update transform efficiently
    }
}
```

## Performance Characteristics

### ObjectSet vs Standard Set
- **Faster Iteration**: ObjectSet maintains an ordered list for O(1) per-element iteration
- **Efficient Memory Usage**: Less memory overhead than standard hash sets
- **Best For**: Iterating over collections where elements change infrequently

### IntMap vs HashMap<int, T>
- **Better Performance**: Optimized for int keys with faster hash computation
- **Cache Friendly**: More efficient memory layout for int-based lookups
- **Best For**: Entity IDs, indices, and other integer-based mappings

### IntFloatMap vs HashMap<int, float>
- **Specialized Operations**: Optimized specifically for int->float mappings
- **Reduced Overhead**: Less memory overhead than generic map implementations
- **Best For**: Game stats, properties, weights, and other float values indexed by integer IDs

## Memory Management

All collections follow RAII principles and handle memory automatically:

- Collections grow dynamically as needed
- Use `Clear()` to reset without destroying the container
- Collections are efficient for frequent additions/removals
- Consider initial capacity for collections with predictable sizes

## Thread Safety

The collections are **not thread-safe** by default. For multi-threaded access:

- Use external synchronization (mutexes)
- Consider separate collections per thread
- Use lock-free alternatives for high-performance requirements

## Integration with ECS

These collections integrate well with ECS (Entity-Component-System) architectures:

- IntMap for component storage: `IntMap<ComponentType>`
- ObjectSet for entity groups: `ObjectSet<EntityType>`
- IntFloatMap for numeric properties: `IntFloatMap`

## Common Use Cases

1. **Entity Management**: Store and manage game entities efficiently
2. **Component Storage**: Store ECS components with integer entity IDs
3. **Resource Lookups**: Fast lookup of resources by ID
4. **Game State**: Store game properties and values efficiently
5. **Particle Systems**: Manage particle data efficiently
6. **Animation Systems**: Store animation parameters and states

## Tips

1. **Choose the Right Collection**: Use specialized collections when possible rather than generic ones
2. **Pre-size When Possible**: Use constructor with initial capacity when size is predictable
3. **Minimize Allocations**: Reuse collections and clear rather than recreating
4. **Use GetPtr for Updates**: Use `GetPtr` instead of `Get` when updating values
5. **Batch Operations**: Use `GetKeys()` and `GetValues()` for batch operations
6. **Performance Test**: Benchmark with your specific use case as performance can vary