#include "core.h"
#include <stdio.h>

void GameInit(void)
{
	InitWindow(1280, 720, "Who Stole The Sun");
}

void GameLoopOneIteration(void)
{
	BeginDrawing();
	ClearBackground(RAYWHITE);
	DrawText("Hello, world! :)", 500, 300, 40, GRAY);
	EndDrawing();
}