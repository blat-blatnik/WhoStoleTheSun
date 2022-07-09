#include "../core.h"

#define COMMAND(codepoint) -codepoint
#define IS_COMMAND(codepoint) (codepoint < 0)

ENUM(Style)
{
	REGULAR     = 0,
	BOLD        = 1 << 0,
	ITALIC      = 1 << 1,
	BOLD_ITALIC = BOLD | ITALIC,
	STYLE_ENUM_COUNT
};

STRUCT(Word)
{
	int start;
	int length;
	float width;
};

static float GetAdvance(Font font, float fontSize, int glyphIndex)
{
	float scaleFactor = fontSize / font.baseSize;
	if (font.glyphs[glyphIndex].advanceX == 0)
		return scaleFactor * font.recs[glyphIndex].width;
	else
		return scaleFactor * font.glyphs[glyphIndex].advanceX;
}

static List(int) ConvertToCodepoints(const char *text, int length)
{
	List(int) codepoints = NULL;

	int lastNonPauseCodepoint = 0;
	for (int i = 0; i < length;)
	{
		int advance;
		int codepoint = GetCodepoint(text + i, &advance);
		if (!codepoint)
			break;
		i += advance;

		int lastIndex = ListCount(codepoints) - 1;
		bool isEscaped = lastIndex > 0 and codepoints[lastIndex] == COMMAND('\\');

		if (codepoint == '\\' and not isEscaped)
			ListAdd(&codepoints, COMMAND('\\'));
		else if (codepoint == '[' and not isEscaped)
			ListAdd(&codepoints, COMMAND('['));
		else if (codepoint == ']' and not isEscaped)
			ListAdd(&codepoints, COMMAND('}'));
		else if (codepoint == '|' and not isEscaped)
			ListAdd(&codepoints, COMMAND('|'));
		else if (codepoint == '_' and not isEscaped)
			ListAdd(&codepoints, COMMAND('_'));
		else if (codepoint == '*' and not isEscaped)
			ListAdd(&codepoints, COMMAND('*'));
		else if (codepoint == '`' and not isEscaped)
			ListAdd(&codepoints, COMMAND('`'));
		else if (codepoint == ' ' and isEscaped)
			codepoints[lastIndex] = ' ';
		else if (codepoint == 'n' and isEscaped)
			codepoints[lastIndex] = '\n';
		else if (codepoint == '[' and isEscaped)
			codepoints[lastIndex] = '[';
		else if (codepoint == ']' and isEscaped)
			codepoints[lastIndex] = ']';
		else if (codepoint == '|' and isEscaped)
			codepoints[lastIndex] = '|';
		else if (codepoint == '_' and isEscaped)
			codepoints[lastIndex] = '_';
		else if (codepoint == '*' and isEscaped)
			codepoints[lastIndex] = '*';
		else if (codepoint == '`' and isEscaped)
			codepoints[lastIndex] = '`';
		else if (codepoint == '\\' and isEscaped)
			codepoints[lastIndex] = '\\';
		else if (codepoint == ' ' and not isEscaped and lastNonPauseCodepoint == ' ')
			ListAdd(&codepoints, COMMAND('`'));
		else if (codepoint == ',' and not isEscaped)
		{
			ListAdd(&codepoints, codepoint);
			ListAdd(&codepoints, COMMAND('`'));
			ListAdd(&codepoints, COMMAND('`'));
		}
		else if ((codepoint == '.' or codepoint == '!' or codepoint == '?') and not isEscaped)
		{
			ListAdd(&codepoints, codepoint);
			ListAdd(&codepoints, COMMAND('`'));
			ListAdd(&codepoints, COMMAND('`'));
			ListAdd(&codepoints, COMMAND('`'));
			ListAdd(&codepoints, COMMAND('`'));
		}
		else
			ListAdd(&codepoints, codepoint);

		int lastCodepoint = codepoints[ListCount(codepoints) - 1];
		if (lastCodepoint != COMMAND('`'))
			lastNonPauseCodepoint = lastCodepoint;
	}

	// Remove pauses at the end.
	while (ListCount(codepoints) > 0 and codepoints[ListCount(codepoints) - 1] == COMMAND('`'))
		ListPop(&codepoints);

	return codepoints;
}

static float MeasureDuration(List(int) codepoints)
{
	int numCodepoints = ListCount(codepoints);
	int duration = 0;
	for (int i = 0; i < numCodepoints; ++i)
	{
		int codepoint = codepoints[i];
		if (not IS_COMMAND(codepoint) or codepoint == COMMAND('`'))
			++duration;

		if (codepoint == COMMAND('['))
		{
			while (i < numCodepoints and codepoint != COMMAND(']'))
				codepoint = codepoints[++i];
		}
	}

	return (float)duration;
}

static bool IsWhitespace(int codepoint)
{
	return codepoint < 128 && CharIsWhitespace((char)codepoint);
}

Script LoadScript(const char *path, Font font)
{
	Script script = { .text = LoadFileText(path), .font = font };
	if (not script.text)
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

			if (not CharIsWhitespace(text[cursor]))
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
						if (not CharIsWhitespace(line[j]))
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
		if (not foundSpeaker)
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
		paragraph.codepoints = ConvertToCodepoints(text, textLength);
		paragraph.duration = MeasureDuration(paragraph.codepoints);
		ListAdd(&script.paragraphs, paragraph);
	}
}

void UnloadScript(Script *script)
{
	// @TODO
}

void DrawParagraph(Script script, int paragraphIndex, Rectangle textBox, float fontSize, Color color, float time)
{
	int mark = TempMark();
	{
		paragraphIndex = ClampInt(paragraphIndex, 0, ListCount(script.paragraphs) - 1);
		Paragraph paragraph = script.paragraphs[paragraphIndex];
		List(int) codepoints = paragraph.codepoints;
		int numCodepoints = ListCount(codepoints);
		
		Font fonts[STYLE_ENUM_COUNT];
		fonts[REGULAR] = script.font;
		fonts[BOLD] = script.font;
		fonts[ITALIC] = script.font;
		fonts[BOLD_ITALIC] = script.font;

		float x = textBox.x;
		float y = textBox.y;
		float t = 0;
		Style style = REGULAR;
		bool group = false;
		float maxX = textBox.x + textBox.width;

		for (int i = 0; i < numCodepoints and (group or t < time); ++i)
		{
			int codepoint = codepoints[i];
			if (codepoint == COMMAND('['))
			{
				// @TODO Handle expressions.
				while (i < numCodepoints and codepoint != COMMAND(']'))
				{
					codepoint = codepoints[++i];
				}
			}
			else if (codepoint == COMMAND('*'))
			{
				style ^= BOLD;
			}
			else if (codepoint == COMMAND('_'))
			{
				style ^= ITALIC;
			}
			else if (codepoint == COMMAND('|'))
			{
				group = not group;
			}
			else if (IsWhitespace(codepoint) or codepoint == COMMAND('`'))
			{
				t += 1;
				if (codepoint == '\n')
				{
					x = textBox.x;
					y += GetLineHeight(fonts[style], fontSize);
				}
				else if (codepoint != COMMAND('`'))
				{
					Font font = fonts[style];
					int index = GetGlyphIndex(font, codepoint);
					x += GetAdvance(font, fontSize, index);
					if (x > maxX)
					{
						x = textBox.x;
						y += GetLineHeight(fonts[style], fontSize);
					}
				}
			}
			else
			{
				// Check if the word will fit on the line, and if it doesn't, break the line before drawing the word.
				float width = 0;
				Style backupStyle = style;
				for (int j = i; j < numCodepoints and not IsWhitespace(codepoints[j]) and codepoints[j] != COMMAND('['); ++j)
				{
					codepoint = codepoints[j];
					if (codepoint == COMMAND('*'))
						style ^= BOLD;
					else if (codepoint == COMMAND('_'))
						style ^= ITALIC;
					else if (not IS_COMMAND(codepoint))
					{
						Font font = fonts[style];
						int index = GetGlyphIndex(font, codepoint);
						width += GetAdvance(font, fontSize, index);
					}
				}

				// Word is too long to fit onto current line. Break the line.
				if (x + width > maxX)
				{
					x = textBox.x;
					y += GetLineHeight(fonts[style], fontSize);
				}

				style = backupStyle;
				{
					Font font = fonts[style];
					int index = GetGlyphIndex(font, codepoint);
					DrawTextCodepoint(font, codepoint, (Vector2) { x, y }, fontSize, color);
					x += GetAdvance(font, fontSize, index);
					t += 1;
				}
			}
		}
	}
	TempReset(mark);
}
