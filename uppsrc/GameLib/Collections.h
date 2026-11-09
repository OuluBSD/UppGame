#ifndef UPP_GAME_COLLECTIONS_H
#define UPP_GAME_COLLECTIONS_H

#include <Core/Core.h>
#include <Vector/Vector.h>
#include <HashMap/HashMap.h>

NAMESPACE_UPP

// ObjectSet - An optimized set for objects with fast iteration
// Based on libgdx's ObjectSet which provides faster iteration than standard hash sets
template<typename T>
class ObjectSet {
private:
    HashMap<T, bool> map;  // Using bool as dummy value
    Vector<T> ordered_keys;  // For fast iteration
    bool keys_dirty = true;  // Track if ordered_keys needs updating

public:
    ObjectSet() {}
    
    ObjectSet(int initialCapacity) {
        map.SetCapacity(initialCapacity);
        ordered_keys.Reserve(initialCapacity);
    }
    
    // Add an element to the set
    void Put(const T& key) {
        if (!map.Contains(key)) {
            map.GetAdd(key) = true;
            keys_dirty = true;
        }
    }
    
    // Alias for Put to match U++ conventions
    void Add(const T& key) {
        Put(key);
    }
    
    // Remove an element from the set
    bool Remove(const T& key) {
        if (map.Remove(key)) {
            keys_dirty = true;
            return true;
        }
        return false;
    }
    
    // Check if the set contains an element
    bool Contains(const T& key) const {
        return map.Contains(key);
    }
    
    // Clear the set
    void Clear() {
        map.Clear();
        ordered_keys.Clear();
        keys_dirty = false;
    }
    
    // Get the size of the set
    int GetCount() const {
        return map.GetCount();
    }
    
    // Check if the set is empty
    bool IsEmpty() const {
        return map.IsEmpty();
    }
    
    // Get all keys for fast iteration
    const Vector<T>& GetKeys() {
        if (keys_dirty) {
            RebuildKeyOrder();
        }
        return ordered_keys;
    }
    
    // Iterator interface for range-based for loops
    typename Vector<T>::iterator begin() {
        if (keys_dirty) {
            RebuildKeyOrder();
        }
        return ordered_keys.begin();
    }
    
    typename Vector<T>::iterator end() {
        if (keys_dirty) {
            RebuildKeyOrder();
        }
        return ordered_keys.end();
    }
    
    typename Vector<T>::const_iterator begin() const {
        if (keys_dirty) {
            const_cast<ObjectSet<T>*>(this)->RebuildKeyOrder();
        }
        return ordered_keys.begin();
    }
    
    typename Vector<T>::const_iterator end() const {
        if (keys_dirty) {
            const_cast<ObjectSet<T>*>(this)->RebuildKeyOrder();
        }
        return ordered_keys.end();
    }

private:
    void RebuildKeyOrder() {
        ordered_keys.Clear();
        for (const auto& pair : map) {
            ordered_keys.Add(pair.key);
        }
        keys_dirty = false;
    }
};

// IntMap - A map from int to a value type with better performance
template<typename T>
class IntMap {
private:
    HashMap<int, T> map;

public:
    IntMap() {}
    
    IntMap(int initialCapacity) {
        map.SetCapacity(initialCapacity);
    }
    
    // Put a value in the map
    void Put(int key, const T& value) {
        map.GetAdd(key) = value;
    }
    
    // Alias for Put to match U++ conventions
    void Add(int key, const T& value) {
        Put(key, value);
    }
    
    // Get a value from the map
    T Get(int key, const T& defaultValue = T()) const {
        const T* value = map.Get(key);
        return value ? *value : defaultValue;
    }
    
    // Get pointer to value, or nullptr if not found
    const T* GetPtr(int key) const {
        return map.Get(key);
    }
    
    T* GetPtr(int key) {
        return map.Get(key);
    }
    
    // Check if the map contains a key
    bool Contains(int key) const {
        return map.Contains(key);
    }
    
    // Remove a key from the map
    bool Remove(int key) {
        return map.Remove(key);
    }
    
    // Clear the map
    void Clear() {
        map.Clear();
    }
    
    // Get the size of the map
    int GetCount() const {
        return map.GetCount();
    }
    
    // Check if the map is empty
    bool IsEmpty() const {
        return map.IsEmpty();
    }
    
    // Iterator interface
    auto begin() { return map.begin(); }
    auto end() { return map.end(); }
    auto begin() const { return map.begin(); }
    auto end() const { return map.end(); }
    
    // Get all values
    Vector<T> GetValues() const {
        Vector<T> values;
        for (const auto& pair : map) {
            values.Add(pair.value);
        }
        return values;
    }
    
    // Get all keys
    Vector<int> GetKeys() const {
        Vector<int> keys;
        for (const auto& pair : map) {
            keys.Add(pair.key);
        }
        return keys;
    }
};

// IntFloatMap - A specialized map from int to float with optimized performance
class IntFloatMap {
private:
    HashMap<int, double> map;  // Using double for better precision

public:
    IntFloatMap() {}
    
    IntFloatMap(int initialCapacity) {
        map.SetCapacity(initialCapacity);
    }
    
    // Put a value in the map
    void Put(int key, double value) {
        map.GetAdd(key) = value;
    }
    
    // Get a value from the map
    double Get(int key, double defaultValue = 0.0) const {
        const double* value = map.Get(key);
        return value ? *value : defaultValue;
    }
    
    // Get pointer to value, or nullptr if not found
    const double* GetPtr(int key) const {
        return map.Get(key);
    }
    
    double* GetPtr(int key) {
        return map.Get(key);
    }
    
    // Check if the map contains a key
    bool Contains(int key) const {
        return map.Contains(key);
    }
    
    // Remove a key from the map
    bool Remove(int key) {
        return map.Remove(key);
    }
    
    // Clear the map
    void Clear() {
        map.Clear();
    }
    
    // Get the size of the map
    int GetCount() const {
        return map.GetCount();
    }
    
    // Check if the map is empty
    bool IsEmpty() const {
        return map.IsEmpty();
    }
    
    // Iterator interface
    auto begin() { return map.begin(); }
    auto end() { return map.end(); }
    auto begin() const { return map.begin(); }
    auto end() const { return map.end(); }
    
    // Get all values
    Vector<double> GetValues() const {
        Vector<double> values;
        for (const auto& pair : map) {
            values.Add(pair.value);
        }
        return values;
    }
    
    // Get all keys
    Vector<int> GetKeys() const {
        Vector<int> keys;
        for (const auto& pair : map) {
            keys.Add(pair.key);
        }
        return keys;
    }
};

// IntIntMap - A specialized map from int to int with optimized performance
class IntIntMap {
private:
    HashMap<int, int> map;

public:
    IntIntMap() {}
    
    IntIntMap(int initialCapacity) {
        map.SetCapacity(initialCapacity);
    }
    
    // Put a value in the map
    void Put(int key, int value) {
        map.GetAdd(key) = value;
    }
    
    // Get a value from the map
    int Get(int key, int defaultValue = 0) const {
        const int* value = map.Get(key);
        return value ? *value : defaultValue;
    }
    
    // Get pointer to value, or nullptr if not found
    const int* GetPtr(int key) const {
        return map.Get(key);
    }
    
    int* GetPtr(int key) {
        return map.Get(key);
    }
    
    // Check if the map contains a key
    bool Contains(int key) const {
        return map.Contains(key);
    }
    
    // Remove a key from the map
    bool Remove(int key) {
        return map.Remove(key);
    }
    
    // Clear the map
    void Clear() {
        map.Clear();
    }
    
    // Get the size of the map
    int GetCount() const {
        return map.GetCount();
    }
    
    // Check if the map is empty
    bool IsEmpty() const {
        return map.IsEmpty();
    }
    
    // Iterator interface
    auto begin() { return map.begin(); }
    auto end() { return map.end(); }
    auto begin() const { return map.begin(); }
    auto end() const { return map.end(); }
    
    // Get all values
    Vector<int> GetValues() const {
        Vector<int> values;
        for (const auto& pair : map) {
            values.Add(pair.value);
        }
        return values;
    }
    
    // Get all keys
    Vector<int> GetKeys() const {
        Vector<int> keys;
        for (const auto& pair : map) {
            keys.Add(pair.key);
        }
        return keys;
    }
};

// ObjectMap - A map from objects to objects optimized for games
template<typename K, typename V>
class ObjectMap {
private:
    HashMap<K, V> map;

public:
    ObjectMap() {}
    
    ObjectMap(int initialCapacity) {
        map.SetCapacity(initialCapacity);
    }
    
    // Put a key-value pair in the map
    void Put(const K& key, const V& value) {
        map.GetAdd(key) = value;
    }
    
    // Alias for Put to match U++ conventions
    void Add(const K& key, const V& value) {
        Put(key, value);
    }
    
    // Get a value from the map
    V Get(const K& key, const V& defaultValue = V()) const {
        const V* value = map.Get(key);
        return value ? *value : defaultValue;
    }
    
    // Get pointer to value
    const V* GetPtr(const K& key) const {
        return map.Get(key);
    }
    
    V* GetPtr(const K& key) {
        return map.Get(key);
    }
    
    // Check if the map contains a key
    bool Contains(const K& key) const {
        return map.Contains(key);
    }
    
    // Remove a key from the map
    bool Remove(const K& key) {
        return map.Remove(key);
    }
    
    // Clear the map
    void Clear() {
        map.Clear();
    }
    
    // Get the size of the map
    int GetCount() const {
        return map.GetCount();
    }
    
    // Check if the map is empty
    bool IsEmpty() const {
        return map.IsEmpty();
    }
    
    // Iterator interface
    auto begin() { return map.begin(); }
    auto end() { return map.end(); }
    auto begin() const { return map.begin(); }
    auto end() const { return map.end(); }
    
    // Get all values
    Vector<V> GetValues() const {
        Vector<V> values;
        for (const auto& pair : map) {
            values.Add(pair.value);
        }
        return values;
    }
    
    // Get all keys
    Vector<K> GetKeys() const {
        Vector<K> keys;
        for (const auto& pair : map) {
            keys.Add(pair.key);
        }
        return keys;
    }
};

END_UPP_NAMESPACE

#endif