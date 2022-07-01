#include "../core.h"

bool CharIsWhitespace(char c)
{
	return c == ' ' or c == '\t' or c == '\n' or c == '\r' or c == '\v';
}

char CharToLowercase(char c)
{
	if (c >= 'A' and c <= 'Z')
		return 'a' + (c - 'A');
	else
		return c;
}

char CharToUppercase(char c)
{
	if (c >= 'a' and c <= 'z')
		return 'A' + (c - 'a');
	else
		return c;
}