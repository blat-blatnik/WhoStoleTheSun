#pragma once

#include "core.h"

//
// List
//

// Use this to declare lists, e.g. List(int) myList = NULL; You can also do int *myList = NULL, but this makes it more distinct.
#define List(T) T*

// Sets the allocator used by the list. By default, lists use MemRealloc and MemFree (heap allocation).
void ListSetAllocator(List(void) *listPointer, void *(*realloc)(void *block, int newSize), void(*free)(void *block));

// Returns the number of items in the list.
int ListCount(const List(void) list);

// Returns the capacity of the list, which is the number of items the list can hold before having to resize.
int ListCapacity(const List(void) list);

// Deallocates all memory held by the list.
void ListDestroy(List(void) *listPointer);

// Ensures that the list has space for at least the given number of elements.
#define ListReserve(listPointer, neededCapacity)\
	private_ListReserve((listPointer), (neededCapacity), sizeof (*listPointer)[0])

// Adds an item to the list.
#define ListAdd(listPointer, item) do{\
	int private_index = ListCount(*(listPointer));\
	ListReserve((listPointer), private_index + 1);\
	(*(listPointer))[private_index] = (item);\
	++((int *)(*(listPointer)))[-1];\
}while(0)

// Adds space for one more item in the list, and returns a pointer to the newly allocated item.
// This can be more efficient and convenient than ListAdd for larger structures.
#define ListReserveOneItem(listPointer)(\
	ListReserve((listPointer), ListCount(*listPointer) + 1),\
	++((int *)(*(listPointer)))[-1],\
	(*listPointer) + ListCount(*listPointer) - 1)

// Removes the last item in the list and returns it.
#define ListPop(listPointer)\
	(private_ListPop(listPointer), (*listPointer)[ListCount(*listPointer)])

// Removes the item at the given index in the list by swapping it with the last item in the list.
// This will destroy the order of the list, but if you don't care about the order, it's very fast.
#define ListSwapRemove(listPointer, index) do{\
	if ((index) < ListCount(*(listPointer)))\
		(*listPointer)[index] = (*listPointer)[--((int *)(*listPointer))[-1]];\
}while(0);\

// Implementation details..
void private_ListReserve(List(void) *listPointer, int neededCapacity, int sizeOfOneItem);
void private_ListPop(List(void) *listPointer);

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
