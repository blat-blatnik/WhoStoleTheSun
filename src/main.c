#include "core.h"
#include "text.h"

Texture *test;
Font roboto;

void GameInit(void)
{
	InitWindow(1280, 720, "Who Stole The Sun");
	SetTargetFPS(60);

	int ascii[128];
	for (int i = 0; i < COUNTOF(ascii); ++i)
		ascii[i] = i;
	roboto = LoadFontEx("res/Roboto.ttf", 32, ascii, COUNTOF(ascii));

	test = LoadTextureAndTrackChanges("res/test.png");
}

void DrawAnimatedTextBox(Font font, Rectangle textBox, float fontSize, Color color, const char *string)
{
	//DrawRectangleRec(textBox, MAGENTA);
	
}

void GameLoopOneIteration(void)
{
	HotReloadAllTrackedItems();
	TempReset(0);

	BeginDrawing();
	{
		ClearBackground(BLACK);
		DrawTexture(*test, 50, 50, WHITE);
		DrawFormat(roboto, 500, 300, 32, WHITE, "Hello, sailor!\nWho killed captain Alex?");

		Rectangle textBox = { 500, 100, 400, 600 };
		DrawAnimatedTextBox(roboto, textBox, 32, BLACK, "I have no clue what the hell is going on...");
	}
	EndDrawing();
}