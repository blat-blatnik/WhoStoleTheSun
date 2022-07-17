#include "../core.h"

int ReadInt(BinaryStream *stream)
{
	int result = 0;
	int bytesRemaining = stream->size - stream->cursor;
	if (bytesRemaining < sizeof result)
		return 0;

	CopyBytes(&result, (char *)stream->buffer + stream->cursor, sizeof result);
	stream->cursor += 4;
	return result;
}

float ReadFloat(BinaryStream *stream)
{
	float result = 0;
	int bytesRemaining = stream->size - stream->cursor;
	if (bytesRemaining < sizeof result)
		return 0;

	CopyBytes(&result, (char *)stream->buffer + stream->cursor, sizeof result);
	stream->cursor += 4;
	return result;
}

const char *ReadString(BinaryStream *stream)
{
	const char *result = (char *)stream->buffer + stream->cursor;
	int bytesRemaining = stream->size - stream->cursor;
	int length;
	for (length = 0; length < bytesRemaining; ++length)
		if (not result[length])
			break;

	if (length >= bytesRemaining)
	{
		stream->cursor = stream->size;
		return NULL; // No 0 terminator for this string.
	}
	
	stream->cursor += length + 1;
	return result;
}

const void *ReadBytes(BinaryStream *stream, int numBytesToRead)
{
	int bytesRemaining = stream->size - stream->cursor;
	if (bytesRemaining < numBytesToRead)
		return NULL;

	const void *result = (char *)stream->buffer + stream->cursor;
	stream->cursor += numBytesToRead;
	return result;
}

void WriteInt(BinaryStream *stream, int i)
{
	WriteBytes(stream, &i, sizeof i);
}

void WriteFloat(BinaryStream *stream, float f)
{
	WriteBytes(stream, &f, sizeof f);
}

void WriteString(BinaryStream *stream, const char *s)
{
	if (not s)
		s = "";
	int length = StringLength(s);
	WriteBytes(stream, s, length + 1);
}

void WriteBytes(BinaryStream *stream, const void *bytes, int numBytesToWrite)
{
	int bytesRemaining = stream->size - stream->cursor;
	if (bytesRemaining < numBytesToWrite)
		return;

	CopyBytes((char *)stream->buffer + stream->cursor, bytes, numBytesToWrite);
	stream->cursor += numBytesToWrite;
}
