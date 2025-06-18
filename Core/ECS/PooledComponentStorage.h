#pragma once

#include "ComponentStorage.h"
#include "MemoryPool.h"
#include "ECSConfig.h"
#include "../Logger.h"
#include <memory>
#include <vector>
#include <string>

namespace BGE {

// Pooled component storage for efficient memory management
template<typename T>
class PooledComponentStorage : public TypedComponentStorage<T> {
public:
    PooledComponentStorage() 
        : m_pool(std::make_unique<ObjectPool<T>>(ECSConfig::Instance().componentPoolBlockSize)) {
        m_components.reserve(ECSConfig::Instance().componentPoolBlockSize);
    }
    
    ~PooledComponentStorage() override {
        // Clean up all components
        for (T* component : m_components) {
            if (component) {
                m_pool->Deallocate(component);
            }
        }
    }
    
    // Override type-erased methods
    void* GetRaw(size_t index) override {
        if (index >= m_components.size()) {
            return nullptr;
        }
        return m_components[index];
    }
    
    const void* GetRaw(size_t index) const override {
        if (index >= m_components.size()) {
            return nullptr;
        }
        return m_components[index];
    }
    
    size_t Size() const override {
        return m_components.size();
    }
    
    void Remove(size_t index) override {
        if (index >= m_components.size()) {
            BGE_LOG_ERROR("PooledComponentStorage", "Remove index out of bounds: " + std::to_string(index));
            return;
        }
        
        // Return to pool
        if (m_components[index]) {
            m_pool->Deallocate(m_components[index]);
            m_components[index] = nullptr;
        }
        
        // Swap with last if not already last
        if (index != m_components.size() - 1) {
            m_components[index] = m_components.back();
        }
        m_components.pop_back();
    }
    
    void Clear() override {
        for (T* component : m_components) {
            if (component) {
                m_pool->Deallocate(component);
            }
        }
        m_components.clear();
    }
    
    void Reserve(size_t capacity) override {
        m_components.reserve(capacity);
        m_pool->Reserve(capacity);
    }
    
    void MoveFrom(size_t destIndex, size_t srcIndex) override {
        if (destIndex >= m_components.size() || srcIndex >= m_components.size()) {
            BGE_LOG_ERROR("PooledComponentStorage", "MoveFrom indices out of bounds");
            return;
        }
        
        m_components[destIndex] = m_components[srcIndex];
        m_components[srcIndex] = nullptr;
    }
    
    // Add a component using pool allocation
    size_t AddPooled(T&& component) {
        size_t index = m_components.size();
        
        // Allocate from pool
        T* pooledComponent = m_pool->Allocate(std::move(component));
        if (!pooledComponent) {
            BGE_LOG_ERROR("PooledComponentStorage", "Failed to allocate component from pool");
            throw std::bad_alloc();
        }
        
        m_components.push_back(pooledComponent);
        return index;
    }
    
    // Get a component pointer
    T* GetPooled(size_t index) {
        if (index >= m_components.size()) {
            return nullptr;
        }
        return m_components[index];
    }
    
    const T* GetPooled(size_t index) const {
        if (index >= m_components.size()) {
            return nullptr;
        }
        return m_components[index];
    }
    
    // Get memory statistics
    size_t GetPoolCapacity() const {
        return m_pool->GetCapacity();
    }
    
    size_t GetPoolUsed() const {
        return m_pool->GetUsed();
    }
    
    size_t GetPoolAvailable() const {
        return m_pool->GetAvailable();
    }
    
private:
    std::unique_ptr<ObjectPool<T>> m_pool;
    std::vector<T*> m_components;  // Pointers to pooled components
};

// Factory function to create pooled storage
template<typename T>
std::unique_ptr<IComponentStorage> CreatePooledComponentStorage() {
    if (ECSConfig::Instance().enableMemoryPooling) {
        return std::make_unique<PooledComponentStorage<T>>();
    } else {
        // Fall back to regular storage if pooling is disabled
        return std::make_unique<TypedComponentStorage<T>>();
    }
}

} // namespace BGE