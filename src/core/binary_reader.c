#include "../core.h"

int ReadInt(BinaryReader *reader)
{
	int result = 0;
	int bytesRemaining = reader->size - reader->cursor;
	if (bytesRemaining < sizeof result)
		return 0;

	CopyBytes(&result, (char *)reader->buffer + reader->cursor, sizeof result);
	reader->cursor += 4;
	return result;
}

float ReadFloat(BinaryReader *reader)
{
	float result = 0;
	int bytesRemaining = reader->size - reader->cursor;
	if (bytesRemaining < sizeof result)
		return 0;

	CopyBytes(&result, (char *)reader->buffer + reader->cursor, sizeof result);
	reader->cursor += 4;
	return result;
}

const char *ReadCString(BinaryReader *reader)
{
	const char *result = (char *)reader->buffer + reader->cursor;
	int bytesRemaining = reader->size - reader->cursor;
	int length;
	for (length = 0; length < bytesRemaining; ++length)
		if (not result[length])
			break;

	ASSERT(length < bytesRemaining); // No 0 terminator for this string.
	reader->cursor += length + 1;
	return result;
}

void ReadBytes(BinaryReader *reader, void *buffer, int numBytesToRead)
{
	int bytesRemaining = reader->size - reader->cursor;
	if (bytesRemaining < numBytesToRead)
	{
		ZeroBytes(buffer, numBytesToRead);
		return;
	}

	CopyBytes(buffer, (char *)reader->buffer + reader->cursor, numBytesToRead);
	reader->cursor += numBytesToRead;
}