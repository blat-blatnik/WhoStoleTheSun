#include "text.h"

void DrawFormat(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, ...)
{
	va_list args;
	va_start(args, format);
	DrawFormatVa(font, x, y, fontSize, color, format, args);
	va_end(args);
}

void DrawFormatVa(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, va_list args)
{
	int mark = TempMark();
	{
		char *string = TempFormatVa(format, args);
		Vector2 pos = { x, y };
		Vector2 origin = { 0 };
		DrawTextPro(font, string, pos, origin, 0, fontSize, 0.0f, color);
	}
	TempReset(mark);
}