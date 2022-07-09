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
		Word word = { i };
		for (; i < numCodepoints; ++i)
		{
			if (CharIsWhitespace((char)codepoints[i]))
				break;
			++word.length;
		}

		if (word.length > 0)
		{
			for (int j = 0; j < word.length; ++j)
			{
				int index = GetGlyphIndex(font, codepoints[word.start + j]);
				word.width += GetAdvance(font, index);
			}
			ListAdd(&words, word);
		}
	}

	return words;
}

Script LoadScript(const char *path, Font font)
{
	Script script = { .text = LoadFileText(path), .font = font };
	if (!script.text)
	{
		LogError("Failed to load script file '%s'.", path);
		return script;
	}

	int fileCursor = 0;
	for (;;)
	{
		Paragraph paragraph = { 0 };
		
		while (CharIsWhitespace(script.text[fileCursor]))
			++fileCursor;
		char *text = script.text + fileCursor;
		if (!text[0])
			return script;

		// Find out where the paragraph ends: 
		// - either at the end of the text, 
		// - or after 2 consecutive newlines with nothing but whitespace between them.
		int cursor;
		int lastNonWhitespace = 0;
		for (cursor = 1;; ++cursor)
		{
			if (!text[cursor])
				break;

			if (!CharIsWhitespace(text[cursor]))
			{
				lastNonWhitespace = cursor;
			}
			else if (text[cursor] == '\n')
			{
				++cursor;
				bool consecutiveNewline = false;
				while (CharIsWhitespace(text[cursor]))
				{
					if (text[cursor] == '\n')
					{
						consecutiveNewline = true;
						break;
					}
					++cursor;
				}
				if (consecutiveNewline)
					break;
			}
		}
		int textLength = lastNonWhitespace + 1;

		// Extract speaker name.
		bool foundSpeaker = false;
		if (text[0] == '[')
		{
			int speakerStart = 1;
			int speakerEnd = -1;
			char *nextLine = NULL;
			for (int i = speakerStart; i < textLength; ++i)
			{
				if (text[i] == '\n')
					break; // Speaker name has to be on the first line, so if we don't find a ] on the first line we don't have a name.
				if (text[i] == ']')
				{
					// Now we've found the [abc] part, we need to make sure the rest of the line is empty.
					// If the line isn't empty, then this is an expression, not a speaker name.
					char *line = text + i + 1;
					bool onlyWhitespaceAfterSpeaker = true;
					for (int j = 0; j < textLength; ++j)
					{
						if (line[j] == '\n')
						{
							nextLine = line + j + 1; // Record where the actual text starts after the speaker.
							break;
						}
						if (!CharIsWhitespace(line[j]))
						{
							onlyWhitespaceAfterSpeaker = false;
							break;
						}
					}

					if (onlyWhitespaceAfterSpeaker)
					{
						speakerEnd = i;
					}
					break;
				}
			}

			if (speakerEnd != -1)
			{
				foundSpeaker = true;
				int nameLength = speakerEnd - speakerStart;
				if (nameLength > 0)
				{
					paragraph.speaker = MemAlloc(nameLength + 1);
					CopyBytes(paragraph.speaker, text + speakerStart, nameLength);
					paragraph.speaker[nameLength] = 0;
				}
				else paragraph.speaker = NULL; // Use the default name.

				int skip = (int)(nextLine - text);
				text = nextLine;
				textLength -= skip;
			}
		}
		if (!foundSpeaker)
		{
			// If we didn't find a name, the character stays the same between paragraphs.
			const char *previousName = NULL;
			if (script.paragraphs)
				previousName = script.paragraphs[ListCount(script.paragraphs) - 1].speaker;

			if (previousName)
			{
				int nameLength = StringLength(previousName);
				paragraph.speaker = MemAlloc(nameLength + 1);
				CopyBytes(paragraph.speaker, previousName, nameLength);
				paragraph.speaker[nameLength] = 0;
			}
			else paragraph.speaker = NULL;
		}

		fileCursor += cursor;
		paragraph.text = text;
		paragraph.textLength = textLength;
		
		// 1) Convert text to codepoints.
		// @TODO 2) Process escape sequences.
		// @TODO 3) Convert consecutive spaces into a single space and pauses.
		// @TODO 4) Add explicit pauses after punctuation.
		// @TODO 5) Convert command characters to command (negative) codepoints.
		for (int i = 0; i < paragraph.textLength;)
		{
			int codepointLength;
			int codepoint = GetCodepoint(paragraph.text + i, &codepointLength);
			if (!codepoint)
				break;
			i += codepointLength;

			ListAdd(&paragraph.codepoints, codepoint);
		}
		
		// @TODO Caluclate the unscaled duration of the the paragraph.
		paragraph.duration = (float)ListCount(paragraph.codepoints);
		ListAdd(&script.paragraphs, paragraph);
	}
}

void UnloadScript(Script *script)
{
	// @TODO
}

void DrawParagraph(Paragraph paragraph, Font font, Rectangle textBox, float fontSize, Color color, float t)
{
	int mark = TempMark();
	{
		List(int) codepoints = paragraph.codepoints;
		List(Word) words = SplitIntoWords(font, codepoints);
		if (ListCount(words) == 0)
			return;

		float scaleFactor = fontSize / font.baseSize;
		float yAdvance = font.baseSize * scaleFactor;
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
				if (codepoint == '\n')
				{
					penX = textBox.x;
					penY += yAdvance;
				}
				else
				{
					int index = GetGlyphIndex(font, codepoint);
					DrawTextCodepoint(font, codepoint, (Vector2) { penX, penY }, fontSize, color);
					penX += scaleFactor * GetAdvance(font, index);
				}
			}

			if (penX + word.width > maxX)
			{
				penX = textBox.x;
				penY += yAdvance;
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