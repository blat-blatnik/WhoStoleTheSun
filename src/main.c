#include "lib/raylib.h"

int main(void)
{
	InitWindow(1280, 720, "Who Stole The Sun");

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("Hello, world! :)", 500, 300, 40, GRAY);
		EndDrawing();
	}

	CloseWindow();
}