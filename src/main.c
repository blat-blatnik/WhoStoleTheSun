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

void GameLoopOneIteration(void)
{
	HotReloadAllTrackedItems();
	TempReset(0);

	BeginDrawing();
	{
		ClearBackground(RAYWHITE);
		DrawTexture(*test, 50, 50, WHITE);
		DrawFormat(roboto, 500, 300, 32, BLACK, "Hello, sailor!\nWho killed captain Alex?");
	}
	EndDrawing();
}