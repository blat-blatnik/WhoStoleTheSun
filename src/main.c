#include "lib/raylib.h"

int main(void)
{
	InitWindow(1280, 720, "Who Stole The Sun");

	Vector2 x = { 1, 2 };
	Vector3 y = { 1, 2, 3 };
	Vector4 z = { 1, 2, 3, 4 };
	Matrix w = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("Hello, world! :)", 500, 300, 40, GRAY);
		EndDrawing();
	}

	CloseWindow();
}