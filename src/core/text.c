#include "../core.h"

Font LoadFontAscii(const char *path, int fontSize)
{
	int ascii[128];
	for (int i = 0; i < COUNTOF(ascii); ++i)
		ascii[i] = i;
	return LoadFontEx(path, fontSize, ascii, COUNTOF(ascii));
}

float GetLineHeight(Font font, float fontSize)
{
	return font.baseSize * (fontSize / font.baseSize);
}

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
		DrawTextPro(font, string, pos, origin, 0, fontSize, 0, color);
	}
	TempReset(mark);
}

void DrawFormatCentered(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, ...)
{
	va_list args;
	va_start(args, format);
	DrawFormatCenteredVa(font, x, y, fontSize, color, format, args);
	va_end(args);
}

void DrawFormatCenteredVa(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, va_list args)
{
	int mark = TempMark();
	{
		char *string = TempFormatVa(format, args);
		Vector2 size = MeasureTextEx(font, string, fontSize, 0);
		Vector2 pos = { x - size.x / 2, y - size.y / 2 };
		Vector2 origin = { 0 };
		DrawTextPro(font, string, pos, origin, 0, fontSize, 0, color);
	}
	TempReset(mark);
}
