#include "../allocators.h"
#include <string.h>

#define ALIGNMENT 16
#define MASK ((uintptr_t)(ALIGNMENT - 1))
#define SLAB_SIZE_GRANULARITY KILOBYTES(64)

STRUCT(Header)
{
	int size;
	char pad[8]; // This is just here to keep alignment to 16 bytes.
	char magic[4]; // This always comes right before an allocation. If it gets modified, we know there must have been an underrun.
};

STRUCT(Footer)
{
	char magic[4]; // This always comes right after an allocation. If it gets modified, we know there must have been an overrun.
};

static bool IsCorrupted(SlabAllocator *allocator, void *block)
{
	Header *header = (Header *)block - 1;
	Footer *footer = (Footer *)((char *)block + header->size);
	return
		not BytesEqual(header->magic, allocator->magic, sizeof header->magic) or
		not BytesEqual(footer->magic, allocator->magic, sizeof footer->magic);
}

void *AllocateFromSlabAllocator(SlabAllocator *allocator, int numBytes)
{
	if (numBytes < 0)
		numBytes = 0;

	for (;;)
	{
		uintptr_t unaligned = (uintptr_t)allocator->slab->memory + allocator->slab->cursor;
		uintptr_t aligned = (unaligned + MASK) & (~MASK);
		int needed = (int)((aligned - unaligned) + sizeof(Header) + numBytes + sizeof(Footer));
		int remaining = allocator->slab->capacity - allocator->slab->cursor;
		if (needed <= remaining)
		{
			// Fast path: The current slab has enough room for the allocation, we can just do a pointer bump. 
			// We should be here 99% of the time.
			allocator->cursor += needed;
			allocator->slab->cursor += needed;

			Header *header = (Header *)aligned;
			void *block = header + 1;
			Footer *footer = (Footer *)((char *)block + numBytes);

			header->size = numBytes;
			CopyBytes(header->magic, allocator->magic, sizeof header->magic);
			CopyBytes(footer->magic, allocator->magic, sizeof footer->magic);
			return block;
		}

		// Slow path: The current slab doesn't have enough leftover space, we need to search for a slab that does.
		Slab *next = allocator->slab->next;
		if (not next)
		{
			// Slowest path: None of the slabs have enough space so we need to allocate new ones.
			int worstCase = ALIGNMENT - 1 + sizeof(Header) + numBytes + sizeof(Footer);
			int consecutiveSlabs = (worstCase + SLAB_SIZE_GRANULARITY - 1) / SLAB_SIZE_GRANULARITY;

			next = MemAlloc(sizeof next[0] + consecutiveSlabs * SLAB_SIZE_GRANULARITY);
			next->capacity = consecutiveSlabs * SLAB_SIZE_GRANULARITY;
			next->cursor = 0;
			next->memory = (char *)(next + 1);
			next->prev = allocator->slab;
			next->next = NULL;
			allocator->slab->next = next;
		}

		allocator->cursor += remaining;
		allocator->slab->cursor += remaining;
		allocator->slab = next;
	}
}

void *ReallocateFromSlabAllocator(SlabAllocator *allocator, void *block, int numBytes)
{
	if (not block)
		return AllocateFromSlabAllocator(allocator, numBytes);
	if (numBytes < 0)
		numBytes = 0;

	ASSERT(not IsCorrupted(allocator, block));
	Header *header = (Header *)block - 1;

	int newSize = numBytes;
	int oldSize = header->size;
	int delta = newSize - oldSize;
	char *end = (char *)block + oldSize + sizeof(Footer);
	char *top = (char *)allocator->slab->memory + allocator->slab->cursor;

	// If this was the last allocated block, we can reuse it, as long as the new block fits in the current slab.
	if (end == top and allocator->slab->cursor + delta <= allocator->slab->capacity)
	{
		allocator->slab->cursor += delta;
		allocator->cursor += delta;
		header->size += delta;
		Footer *footer = (Footer *)((char *)block + newSize);
		CopyBytes(footer->magic, allocator->magic, sizeof footer->magic);
		ZeroBytes(footer + 1, -delta);
		return block;
	}

	// We can always reuse the block if the new size is smaller.
	if (newSize <= oldSize)
	{
		header->size += delta;
		Footer *footer = (Footer *)((char *)block + newSize);
		CopyBytes(footer->magic, allocator->magic, sizeof footer->magic);
		ZeroBytes(footer + 1, -delta);
		return block;
	}

	void *copy = AllocateFromSlabAllocator(allocator, numBytes);
	int toCopy = newSize;
	if (toCopy > oldSize)
		toCopy = oldSize;
	CopyBytes(copy, block, toCopy);
	return copy;
}

void FreeFromSlabAllocator(SlabAllocator *allocator, void *block)
{
	if (!block)
		return;

	ASSERT(not IsCorrupted(allocator, block));
	Header *header = (Header *)block - 1;
	int size = sizeof(Header) + header->size + sizeof(Footer);

	char *end = (char *)block + header->size + sizeof(Footer);
	char *top = (char *)allocator->slab->memory + allocator->slab->cursor;
	if (end == top)
	{
		allocator->slab->cursor -= size;
		allocator->cursor -= size;
	}

	ZeroBytes(header, size);
}

void ResetSlabAllocator(SlabAllocator *allocator, int cursor)
{
	if (cursor < 0)
		cursor = 0;

	for (;;)
	{
		int remaining = allocator->cursor - cursor;
		if (remaining <= allocator->slab->cursor)
		{
			char *start = (char *)allocator->slab->memory + allocator->slab->cursor - remaining;
			ZeroBytes(start, remaining);
			allocator->slab->cursor -= remaining;
			allocator->cursor = cursor;
			return;
		}

		ZeroBytes(allocator->slab->memory, allocator->slab->cursor);
		allocator->cursor -= allocator->slab->cursor;
		allocator->slab->cursor = 0;
		if (allocator->slab->prev)
			allocator->slab = allocator->slab->prev;
	}
}
