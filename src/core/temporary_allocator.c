#include "../core.h"
#include "../containers.h"
#include <stdio.h>

static char firstSlabMemory[MEGABYTES(1)];

static Slab firstSlab = {
	.memory = firstSlabMemory,
	.capacity = sizeof firstSlabMemory,
};

static SlabAllocator allocator = {
	.magic = { 'T', 'E', 'M', 'P' },
	.slab = &firstSlab
};

void *TempAlloc(int numBytes)
{
	return AllocateFromSlabAllocator(&allocator, numBytes);
}

void *TempRealloc(void *block, int numBytes)
{
	return ReallocateFromSlabAllocator(&allocator, block, numBytes);
}

void TempFree(void *block)
{
	FreeFromSlabAllocator(&allocator, block);
}

int TempMark(void)
{
	return allocator.cursor;
}

void TempReset(int mark)
{
	ResetSlabAllocator(&allocator, mark);
}

void *TempCopy(const void *bytes, int numBytes)
{
	void *copy = TempAlloc(numBytes);
	CopyBytes(copy, bytes, numBytes);
	return copy;
}

char *TempString(const char *string)
{
	if (not string)
		return NULL;

	int length = StringLength(string);
	return TempCopy(string, 1 + length); // +1 so that we also copy the terminating 0.
}

char *TempFormat(FORMAT_STRING format, ...)
{
	va_list args;
	va_start(args, format);
	char *result = TempFormatVa(format, args);
	va_end(args);
	return result;
}

char *TempFormatVa(FORMAT_STRING format, va_list args)
{
	if (not format)
		return TempString("(null)");

	va_list argCopy;
	va_copy(argCopy, args);
	int charsNeeded = vsnprintf(NULL, 0, format, argCopy);
	va_end(argCopy);

	if (charsNeeded < 0)
		return TempString("(error)");

	int bytesNeeded = charsNeeded + 1;
	char *buffer = TempAlloc(bytesNeeded);
	vsnprintf(buffer, (size_t)bytesNeeded, format, args);
	return buffer;
}
