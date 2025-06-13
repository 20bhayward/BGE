#pragma once

#include <cstddef>

namespace BGE {

class Allocator {
public:
    virtual ~Allocator() = default;
    
    virtual void* Allocate(size_t size, size_t alignment = sizeof(void*)) = 0;
    virtual void Deallocate(void* ptr) = 0;
    virtual void Reset() = 0;
    
    virtual size_t GetTotalAllocated() const = 0;
    virtual size_t GetTotalSize() const = 0;
};

class LinearAllocator : public Allocator {
public:
    explicit LinearAllocator(size_t size);
    ~LinearAllocator();
    
    void* Allocate(size_t size, size_t alignment = sizeof(void*)) override;
    void Deallocate(void* ptr) override; // No-op for linear allocator
    void Reset() override;
    
    size_t GetTotalAllocated() const override { return m_offset; }
    size_t GetTotalSize() const override { return m_size; }

private:
    char* m_memory;
    size_t m_size;
    size_t m_offset;
    
    size_t AlignForward(size_t address, size_t alignment);
};

} // namespace BGE