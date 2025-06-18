#pragma once

#include "MemoryPool.h"
#include <vector>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>

namespace BGE {

// Structure-of-Arrays component storage for cache-efficient iteration
template<typename T>
class ComponentStorage {
public:
    static constexpr size_t INITIAL_CAPACITY = 1024;
    
    ComponentStorage() {
        m_data.reserve(INITIAL_CAPACITY);
    }
    
    ~ComponentStorage() = default;
    
    // Add a component and return its index
    size_t Add(T&& component) {
        size_t index = m_data.size();
        m_data.push_back(std::move(component));
        return index;
    }
    
    // Add a component by copy
    size_t Add(const T& component) {
        size_t index = m_data.size();
        m_data.push_back(component);
        return index;
    }
    
    // Emplace a component
    template<typename... Args>
    size_t Emplace(Args&&... args) {
        size_t index = m_data.size();
        m_data.emplace_back(std::forward<Args>(args)...);
        return index;
    }
    
    // Remove a component by swapping with last
    void Remove(size_t index) {
        assert(index < m_data.size());
        if (index != m_data.size() - 1) {
            m_data[index] = std::move(m_data.back());
        }
        m_data.pop_back();
    }
    
    // Get component by index
    T& Get(size_t index) {
        assert(index < m_data.size());
        return m_data[index];
    }
    
    const T& Get(size_t index) const {
        assert(index < m_data.size());
        return m_data[index];
    }
    
    // Direct access to data
    T* GetData() { return m_data.data(); }
    const T* GetData() const { return m_data.data(); }
    
    // Get storage size
    size_t Size() const { return m_data.size(); }
    size_t Capacity() const { return m_data.capacity(); }
    
    // Clear all components
    void Clear() { m_data.clear(); }
    
    // Reserve capacity
    void Reserve(size_t capacity) { m_data.reserve(capacity); }
    
    // Iterator support
    typename std::vector<T>::iterator begin() { return m_data.begin(); }
    typename std::vector<T>::iterator end() { return m_data.end(); }
    typename std::vector<T>::const_iterator begin() const { return m_data.begin(); }
    typename std::vector<T>::const_iterator end() const { return m_data.end(); }
    
private:
    std::vector<T> m_data;
};

// Type-erased component storage interface
class IComponentStorage {
public:
    virtual ~IComponentStorage() = default;
    
    virtual void* GetRaw(size_t index) = 0;
    virtual const void* GetRaw(size_t index) const = 0;
    virtual size_t Size() const = 0;
    virtual void Remove(size_t index) = 0;
    virtual void Clear() = 0;
    virtual void Reserve(size_t capacity) = 0;
    
    // Component operations
    virtual void ConstructAt(void* ptr) = 0;
    virtual void DestructAt(void* ptr) = 0;
    virtual void CopyConstruct(void* dst, const void* src) = 0;
    virtual void MoveConstruct(void* dst, void* src) = 0;
    virtual void MoveFrom(size_t dstIndex, size_t srcIndex) = 0;
};

// Type-erased wrapper for ComponentStorage
template<typename T>
class TypedComponentStorage : public IComponentStorage {
public:
    void* GetRaw(size_t index) override {
        return &m_storage.Get(index);
    }
    
    const void* GetRaw(size_t index) const override {
        return &m_storage.Get(index);
    }
    
    size_t Size() const override {
        return m_storage.Size();
    }
    
    void Remove(size_t index) override {
        m_storage.Remove(index);
    }
    
    void Clear() override {
        m_storage.Clear();
    }
    
    void Reserve(size_t capacity) override {
        m_storage.Reserve(capacity);
    }
    
    void ConstructAt(void* ptr) override {
        new(ptr) T();
    }
    
    void DestructAt(void* ptr) override {
        static_cast<T*>(ptr)->~T();
    }
    
    void CopyConstruct(void* dst, const void* src) override {
        new(dst) T(*static_cast<const T*>(src));
    }
    
    void MoveConstruct(void* dst, void* src) override {
        new(dst) T(std::move(*static_cast<T*>(src)));
    }
    
    void MoveFrom(size_t dstIndex, size_t srcIndex) override {
        m_storage.Get(dstIndex) = std::move(m_storage.Get(srcIndex));
    }
    
    ComponentStorage<T>& GetTypedStorage() { return m_storage; }
    const ComponentStorage<T>& GetTypedStorage() const { return m_storage; }
    
private:
    ComponentStorage<T> m_storage;
};

// Note: PooledComponentStorage has been removed as the base ComponentStorage
// already provides efficient memory management. For pooled allocation,
// use ObjectPool<T> directly in systems that need temporary allocations.

// Factory function to create appropriate storage based on configuration
template<typename T>
std::unique_ptr<IComponentStorage> CreateComponentStorage(bool usePooling = false) {
    // For now, always use TypedComponentStorage since PooledComponentStorage needs more work
    return std::make_unique<TypedComponentStorage<T>>();
}

} // namespace BGE