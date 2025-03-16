#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <omniscript/omniscript_pch.h>

class Allocator {
private:
    static constexpr size_t BLOCK_SIZE = 1024 * 1024; // 1MB blocks
    std::vector<void*> memoryBlocks; // Store allocated memory blocks
    size_t currentOffset = 0;  // Track allocation offset in the current block
    char* currentBlock = nullptr; // Pointer to current block

    // Allocates a new block when needed
    void allocateNewBlock();

public:
    Allocator();
    ~Allocator();

    void* allocate(size_t size);
    void deallocateAll(); // Frees all memory at once
};

#endif
