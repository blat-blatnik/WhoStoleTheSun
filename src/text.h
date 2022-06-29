#pragma once

#include "core.h"

// Draws a formatted string starting at (x, y) and going right and down.
void DrawFormat(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, ...);

// Same as DrawFormat but takes an explicit varargs pack.
void DrawFormatVa(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, va_list args);