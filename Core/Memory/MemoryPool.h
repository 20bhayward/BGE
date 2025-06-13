#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace BGE {

class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t blockCount);
    ~MemoryPool();
    
    void* Allocate();
    void Deallocate(void* ptr);
    
    size_t GetBlockSize() const { return m_blockSize; }
    size_t GetBlockCount() const { return m_blockCount; }
    size_t GetUsedBlocks() const { return m_usedBlocks; }
    bool IsFull() const { return m_usedBlocks >= m_blockCount; }

private:
    size_t m_blockSize;
    size_t m_blockCount;
    size_t m_usedBlocks;
    
    std::unique_ptr<char[]> m_pool;
    std::vector<void*> m_freeList;
    
    void InitializeFreeList();
};

} // namespace BGE