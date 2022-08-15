#include "../core.h"

int ReadInt(BinaryStream *stream)
{
	int result;
	ReadBytesInto(stream, &result, sizeof result);
	return result;
}

float ReadFloat(BinaryStream *stream)
{
	float result;
	ReadBytesInto(stream, &result, sizeof result);
	return result;
}

bool ReadBool(BinaryStream *stream)
{
	unsigned char result;
	ReadBytesInto(stream, &result, sizeof result);
	return result != 0;
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

void ReadBytesInto(BinaryStream *stream, void *buffer, int numBytesToRead)
{
	int bytesRemaining = stream->size - stream->cursor;
	if (bytesRemaining < numBytesToRead)
	{
		ZeroBytes(buffer, numBytesToRead);
		return;
	}

	const void *result = (char *)stream->buffer + stream->cursor;
	stream->cursor += numBytesToRead;
	CopyBytes(buffer, result, numBytesToRead);
}

void WriteInt(BinaryStream *stream, int i)
{
	WriteBytes(stream, &i, sizeof i);
}

void WriteFloat(BinaryStream *stream, float f)
{
	WriteBytes(stream, &f, sizeof f);
}

void WriteBool(BinaryStream *stream, bool b)
{
	unsigned char byte = b ? true : false;
	WriteBytes(stream, &byte, sizeof byte);
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
