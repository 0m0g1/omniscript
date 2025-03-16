#include <omniscript/engine/Allocator.h>

Allocator::Allocator() {
    allocateNewBlock();
}

Allocator::~Allocator() {
    for (void* block : memoryBlocks) {
        std::free(block);
    }
    memoryBlocks.clear();
}

// Allocates a new memory block
void Allocator::allocateNewBlock() {
    currentBlock = static_cast<char*>(std::malloc(BLOCK_SIZE));
    if (!currentBlock) {
        throw std::bad_alloc();
    }
    memoryBlocks.push_back(currentBlock);
    currentOffset = 0;
}

// Allocates memory from the current block
void* Allocator::allocate(size_t size) {
    if (size > BLOCK_SIZE) {
        throw std::bad_alloc(); // Large allocations are not handled
    }

    // If not enough space, allocate a new block
    if (currentOffset + size > BLOCK_SIZE) {
        allocateNewBlock();
    }

    void* ptr = currentBlock + currentOffset;
    currentOffset += size;

    return ptr;
}

// Frees all allocated memory at once
void Allocator::deallocateAll() {
    for (void* block : memoryBlocks) {
        std::free(block);
    }
    memoryBlocks.clear();
    allocateNewBlock();
}
