#include "../core.h"
#include <string.h>
#include <stdio.h>

StringBuilder CreateStringBuilder(char buffer[], int capacity)
{
	ASSERT(buffer or capacity <= 0);

	if (capacity < 0)
		capacity = 0;
	if (capacity > 0)
		buffer[0] = 0;

	return (StringBuilder)
	{
		.buffer = buffer,
		.capacity = capacity,
		.cursor = 0,
	};
}

void AppendChar(StringBuilder *builder, char c)
{
	ASSERT(builder);

	if (builder->cursor + 1 < builder->capacity)
	{
		builder->buffer[builder->cursor++] = c;
		builder->buffer[builder->cursor] = 0;
	}
}

void AppendCharRepeated(StringBuilder *builder, char c, int repeatCount)
{
	ASSERT(builder);
	
	for (int i = 0; i < repeatCount; ++i)
		AppendChar(builder, c);
}

void AppendString(StringBuilder *builder, const char *string)
{
	ASSERT(builder);

	if (string)
		for (int i = 0; string[i]; ++i)
			AppendChar(builder, string[i]);
}

void AppendFormat(StringBuilder *builder, FORMAT_STRING format, ...)
{
	va_list args;
	va_start(args, format);
	AppendFormatVa(builder, format, args);
	va_end(args);
}

void AppendFormatVa(StringBuilder *builder, FORMAT_STRING format, va_list args)
{
	ASSERT(builder);

	int bytesRemaining = builder->capacity - builder->cursor;
	if (bytesRemaining < 1)
		return;

	int charsRemaining = bytesRemaining - 1;
	int charsWritten = vsnprintf(builder->buffer + builder->cursor, bytesRemaining, format ? format : "(null)", args);
	ASSERT(charsWritten >= 0); // sprintf returns negative values on error.
	if (charsWritten > charsRemaining)
		charsWritten = charsRemaining;

	builder->cursor += charsWritten;
}