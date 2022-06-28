#include "../core.h"

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