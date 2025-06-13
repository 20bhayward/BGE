#include "MemoryPool.h"

namespace BGE {

MemoryPool::MemoryPool(size_t blockSize, size_t blockCount)
    : m_blockSize(blockSize), m_blockCount(blockCount), m_usedBlocks(0) {
    
    m_pool = std::make_unique<char[]>(blockSize * blockCount);
    m_freeList.reserve(blockCount);
    InitializeFreeList();
}

MemoryPool::~MemoryPool() = default;

void* MemoryPool::Allocate() {
    if (m_freeList.empty()) {
        return nullptr; // Pool is full
    }
    
    void* ptr = m_freeList.back();
    m_freeList.pop_back();
    ++m_usedBlocks;
    
    return ptr;
}

void MemoryPool::Deallocate(void* ptr) {
    if (!ptr) return;
    
    // Verify ptr is within our pool bounds
    char* charPtr = static_cast<char*>(ptr);
    if (charPtr >= m_pool.get() && charPtr < m_pool.get() + (m_blockSize * m_blockCount)) {
        m_freeList.push_back(ptr);
        --m_usedBlocks;
    }
}

void MemoryPool::InitializeFreeList() {
    for (size_t i = 0; i < m_blockCount; ++i) {
        void* blockPtr = m_pool.get() + (i * m_blockSize);
        m_freeList.push_back(blockPtr);
    }
}

} // namespace BGE