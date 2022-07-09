#include "../core.h"

STRUCT(Header)
{
	void *(*realloc)(void *block, int newSize);
	void (*free)(void *block);
	int capacity;
	int count;
};

static Header *GetHeader(const List(void) list)
{
	return (Header *)list - 1;
}

void ListSetAllocator(List(void) *listPointer, void *(*realloc)(void *block, int newSize), void(*free)(void *block))
{
	ASSERT(not *listPointer); // You can only call ListSetAllocator on a completely empty (NULL) list!

	if (not *listPointer)
		*listPointer = (Header *)realloc(NULL, sizeof(Header)) + 1;

	Header *header = GetHeader(*listPointer);
	header->realloc = realloc;
	header->free = free;
	header->capacity = 0;
	header->count = 0;
}

int ListCount(const List(void) list)
{
	return list ? GetHeader(list)->count : 0;
}

int ListCapacity(const List(void) list)
{
	return list ? GetHeader(list)->capacity : 0;
}

void ListDestroy(List(void) *listPointer)
{
	if (not *listPointer)
		return;

	Header *header = GetHeader(*listPointer);
	if (header->free)
		header->free(header);
	*listPointer = NULL;
}

void private_ListReserve(List(void) *listPointer, int neededCapacity, int sizeOfOneItem)
{
	int capacity = ListCapacity(*listPointer);
	if (capacity >= neededCapacity)
		return;

	if (capacity < 64)
		capacity = 64;
	while (capacity < neededCapacity)
		capacity *= 2;

	if (not *listPointer)
	{
		Header *header = MemRealloc(NULL, sizeof(Header) + capacity * sizeOfOneItem);
		header->realloc = MemRealloc;
		header->free = MemFree;
		header->capacity = capacity;
		header->count = 0;
		*listPointer = header + 1;
	}
	else
	{
		Header *header = GetHeader(*listPointer);
		header = header->realloc(header, sizeof(Header) + capacity * sizeOfOneItem);
		header->capacity = capacity;
		*listPointer = header + 1;
	}
}

void private_ListPop(List(void) *listPointer)
{
	ASSERT(ListCount(*listPointer) > 0); // Can't pop from an empty list.
	
	Header *header = GetHeader(*listPointer);
	header->count--;
}