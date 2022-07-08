#include "../core.h"
#include <stdio.h>
#include <string.h>

int StringLength(const char *string)
{
	size_t length = string ? strlen(string) : 0;
	return length > INT_MAX ? INT_MAX : (int)length;
}

void CopyString(char *to, const char *from, int maxBytesToCopy)
{
	ASSERT(to or maxBytesToCopy <= 0);

	int charsToCopy = StringLength(from);
	if (charsToCopy > maxBytesToCopy - 1)
		charsToCopy = maxBytesToCopy - 1;

	if (charsToCopy > 0)
		memcpy(to, from, (size_t)charsToCopy);
	if (charsToCopy >= 0)
		to[charsToCopy] = 0; // Ensure output is 0 terminated.
}

void FormatString(char *buffer, int capacity, FORMAT_STRING format, ...)
{
	va_list args;
	va_start(args, format);
	FormatStringVa(buffer, capacity, format, args);
	va_end(args);
}

void FormatStringVa(char *buffer, int capacity, FORMAT_STRING format, va_list args)
{
	ASSERT(buffer or capacity <= 0);

	if (capacity < 1)
		return;

	int result = vsnprintf(buffer, (size_t)capacity, format ? format : "(null)", args);
	ASSERT(result >= 0); // sprintf returns negative values on error.
}

bool StringsEqual(const char *a, const char *b)
{
	if (not a) 
		return not b;
	if (not b)
		return false;

	return strcmp(a, b) == 0;
}

bool StringsEqualNocase(const char *a, const char *b)
{
	if (not a)
		return not b;
	if (not b)
		return false;

	for (int i = 0;; ++i)
	{
		if (CharToLowercase(a[i]) != CharToLowercase(b[i]))
			return false;
		if (!a[i])
			return true;
	}
}

void ReplaceChar(char *string, char target, char replacement)
{
	if (!string)
		return;

	for (int i = 0; string[i]; ++i)
		if (string[i] == target)
			string[i] = replacement;
}

char *SkipLeadingWhitespace(const char *string)
{
	if (!string)
		return NULL;

	while (CharIsWhitespace(*string))
		++string;
	return (char *)string;
}
