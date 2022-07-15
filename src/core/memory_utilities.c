#include "../core.h"
#include <string.h>
#include <stdlib.h>

void ZeroBytes(void *bytes, int count)
{
	SetBytes(bytes, 0, count);
}

void SetBytes(void *bytes, unsigned char value, int count)
{
	ASSERT(bytes or count <= 0);

	if (count > 0)
		memset(bytes, value, (size_t)count);
}

void SetInts(int *ints, int value, int count)
{
	ASSERT(ints or count <= 0);

	for (int i = 0; i < count; ++i)
		ints[i] = value;
}

void SetFloats(float *floats, float value, int count)
{
	ASSERT(floats or count <= 0);

	for (int i = 0; i < count; ++i)
		floats[i] = value;
}

void CopyBytes(void *to, const void *from, int numBytes)
{
	ASSERT((to and from) or numBytes <= 0);

	if (numBytes > 0)
		memcpy(to, from, (size_t)numBytes);
}

void SwapBytes(void *a, void *b, int numBytes)
{
	uint8_t *A = a;
	uint8_t *B = b;
	for (int i = 0; i < numBytes; ++i)
	{
		uint8_t temp = A[i];
		A[i] = B[i];
		B[i] = temp;
	}
}

bool BytesEqual(const void *a, const void *b, int numBytes)
{
	ASSERT((a and b) or numBytes <= 0);
	
	if (numBytes <= 0)
		return true;
	return memcmp(a, b, (size_t)numBytes) == 0;
}

void Sort(void *items, int numItems, int sizeofOneItem, int(*compare)(const void *left, const void *right))
{
	ASSERT(compare);
	ASSERT(items or (numItems <= 0 or sizeofOneItem <= 0));

	if (numItems <= 0 or sizeofOneItem <= 0)
		return;

	qsort(items, (size_t)numItems, (size_t)sizeofOneItem, compare);
}
