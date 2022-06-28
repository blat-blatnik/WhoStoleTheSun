#pragma once

#include "core.h"

//
// Slab allocator
//

STRUCT(Slab)
{
	Slab *prev;
	Slab *next;
	void *memory;
	int cursor;
	int capacity;
};

STRUCT(SlabAllocator)
{
	char magic[4];
	Slab *slab;
	int cursor;
};

// Allocates memory from the allocator. The returned pointer will be aligned to a 16-byte boundary.
// The returned memory will be zeroed. If the requested byte count is 0 or negative, a 0 sized valid pointer is returned.
void *AllocateFromSlabAllocator(SlabAllocator *allocator, int numBytes);

// Reallocates a previously allocated memory block with a new size. If the memory block grows, the new bytes will be zeroed.
// If `block` is NULL, the call is equivalent to AllocateFromSlabAllocator(allocator, numBytes).
void *ReallocateFromSlabAllocator(SlabAllocator *allocator, void *block, int numBytes);

// Deallocates a previously allocated memory block. If `block` is NULL this function does nothing.
void FreeFromSlabAllocator(SlabAllocator *allocator, void *block);

// Frees all memory allocated from the allocator after the given cursor.
void ResetSlabAllocator(SlabAllocator *allocator, int cursor);
