#include "Allocator.h"
#include <cstdlib>

namespace BGE {

LinearAllocator::LinearAllocator(size_t size) : m_size(size), m_offset(0) {
    m_memory = static_cast<char*>(std::malloc(size));
}

LinearAllocator::~LinearAllocator() {
    std::free(m_memory);
}

void* LinearAllocator::Allocate(size_t size, size_t alignment) {
    size_t aligned_offset = AlignForward(m_offset, alignment);
    
    if (aligned_offset + size > m_size) {
        return nullptr; // Out of memory
    }
    
    void* ptr = m_memory + aligned_offset;
    m_offset = aligned_offset + size;
    
    return ptr;
}

void LinearAllocator::Deallocate(void* ptr) {
    // No-op for linear allocator
    (void)ptr;
}

void LinearAllocator::Reset() {
    m_offset = 0;
}

size_t LinearAllocator::AlignForward(size_t address, size_t alignment) {
    return (address + alignment - 1) & ~(alignment - 1);
}

} // namespace BGE