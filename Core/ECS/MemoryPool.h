#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <cstddef>
#include <new>

namespace BGE {

// Object pool for efficient allocation/deallocation
template<typename T>
class ObjectPool {
public:
    static constexpr size_t DEFAULT_BLOCK_SIZE = 1024;
    
    explicit ObjectPool(size_t blockSize = DEFAULT_BLOCK_SIZE) 
        : m_blockSize(blockSize) {
        AllocateBlock();
    }
    
    ~ObjectPool() {
        // Destroy all objects
        for (auto& block : m_blocks) {
            delete[] reinterpret_cast<Storage*>(block);
        }
    }
    
    template<typename... Args>
    T* Allocate(Args&&... args) {
        if (m_available.empty()) {
            AllocateBlock();
        }
        
        void* ptr = m_available.front();
        m_available.pop();
        
        // Construct object in-place
        return new(ptr) T(std::forward<Args>(args)...);
    }
    
    void Deallocate(T* obj) {
        if (!obj) return;
        
        // Call destructor
        obj->~T();
        
        // Return to pool
        m_available.push(obj);
    }
    
    size_t GetCapacity() const {
        return m_blocks.size() * m_blockSize;
    }
    
    size_t GetAvailable() const {
        return m_available.size();
    }
    
    size_t GetUsed() const {
        return GetCapacity() - GetAvailable();
    }
    
    void Reserve(size_t count) {
        while (GetCapacity() < count) {
            AllocateBlock();
        }
    }
    
private:
    // Storage that matches T's size and alignment
    struct alignas(T) Storage {
        char data[sizeof(T)];
    };
    
    void AllocateBlock() {
        // Allocate block of storage
        Storage* block = new Storage[m_blockSize];
        m_blocks.push_back(reinterpret_cast<char*>(block));
        
        // Add all slots to available queue
        for (size_t i = 0; i < m_blockSize; ++i) {
            m_available.push(&block[i]);
        }
    }
    
    size_t m_blockSize;
    std::vector<char*> m_blocks;
    std::queue<void*> m_available;
};

// Arena allocator for temporary allocations
class ArenaAllocator {
public:
    static constexpr size_t DEFAULT_ARENA_SIZE = 1024 * 1024; // 1MB
    
    explicit ArenaAllocator(size_t size = DEFAULT_ARENA_SIZE) {
        m_memory = std::make_unique<char[]>(size);
        m_size = size;
        m_offset = 0;
    }
    
    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        // Align offset
        size_t alignedOffset = (m_offset + alignment - 1) & ~(alignment - 1);
        
        // Check if we have space
        if (alignedOffset + size > m_size) {
            return nullptr; // Out of memory
        }
        
        void* ptr = m_memory.get() + alignedOffset;
        m_offset = alignedOffset + size;
        
        return ptr;
    }
    
    void Reset() {
        m_offset = 0;
    }
    
    size_t GetUsed() const { return m_offset; }
    size_t GetCapacity() const { return m_size; }
    size_t GetAvailable() const { return m_size - m_offset; }
    
private:
    std::unique_ptr<char[]> m_memory;
    size_t m_size;
    size_t m_offset;
};

} // namespace BGE