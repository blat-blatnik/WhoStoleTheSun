#include "../core.h"

STRUCT(Word)
{
	int start;
	int length;
	float width;
};

static float GetAdvance(Font font, int glyphIndex)
{
	if (font.glyphs[glyphIndex].advanceX == 0)
		return font.recs[glyphIndex].width;
	else
		return (float)font.glyphs[glyphIndex].advanceX;
}

static List(Word) SplitIntoWords(Font font, List(int) codepoints)
{
	List(Word) words = NULL;
	ListSetAllocator(&words, TempRealloc, TempFree);
	
	int numCodepoints = ListCount(codepoints);
	for (int i = 0; i < numCodepoints; ++i)
	{
		int codepoint = codepoints[i];
		if (CharIsWhitespace((char)codepoint))
			continue;

		Word word = { i, 1 };
		for (; i < numCodepoints; ++i)
		{
			codepoint = codepoints[i];
			if (CharIsWhitespace((char)codepoint))
				break;
			++word.length;
		}

		for (int j = 0; j < word.length; ++j)
		{
			int index = GetGlyphIndex(font, codepoints[word.start + j]);
			word.width += GetAdvance(font, index);
		}

		ListAdd(&words, word);
	}

	return words;
}

static List(int) TextToCodepoints(const char *text)
{
	List(int) codepoints = NULL;
	if (not text)
		return codepoints;

	ListSetAllocator(&codepoints, TempRealloc, TempFree);
	for (int i = 0;; ++i)
	{
		int advance;
		int codepoint = GetCodepoint(text, &advance);
		text += advance;
		if (!codepoint)
			break;

		ListAdd(&codepoints, codepoint);
	}

	return codepoints;
}

Font LoadFontAscii(const char *path, int fontSize)
{
	int ascii[128];
	for (int i = 0; i < COUNTOF(ascii); ++i)
		ascii[i] = i;
	return LoadFontEx(path, fontSize, ascii, COUNTOF(ascii));
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
		DrawTextPro(font, string, pos, origin, 0, fontSize, 0.0f, color);
	}
	TempReset(mark);
}

void DrawAnimatedTextBox(Font font, Rectangle textBox, float fontSize, Color color, float t, const char *string)
{
	DrawRectangleRec(textBox, Darker(RED));
	if (not string)
		return;

	int mark = TempMark();
	{
		List(int) codepoints = TextToCodepoints(string);
		List(Word) words = SplitIntoWords(font, codepoints);
		if (ListCount(words) == 0)
			return;

		float scaleFactor = fontSize / font.baseSize;
		float maxX = textBox.x + textBox.width;
		float penX = textBox.x;
		float penY = textBox.y;

		int i = 0;
		int numWords = ListCount(words);
		for (int wordIndex = 0; wordIndex < numWords; ++wordIndex)
		{
			Word word = words[wordIndex];

			for (; i < word.start; ++i)
			{
				if (i > t)
				{
					TempReset(mark);
					return;
				}

				int codepoint = codepoints[i];
				int index = GetGlyphIndex(font, codepoint);
				DrawTextCodepoint(font, codepoint, (Vector2) { penX, penY }, fontSize, color);
				penX += scaleFactor * GetAdvance(font, index);
			}

			if (penX + word.width > maxX)
			{
				penX = textBox.x;
				penY += font.baseSize;
			}

			for (; i < word.start + word.length; ++i)
			{
				if (i > t)
				{
					TempReset(mark);
					return;
				}

				int codepoint = codepoints[i];
				int index = GetGlyphIndex(font, codepoint);
				DrawTextCodepoint(font, codepoint, (Vector2) { penX, penY }, fontSize, color);
				penX += scaleFactor * GetAdvance(font, index);
			}
		}
	}
	TempReset(mark);
}
