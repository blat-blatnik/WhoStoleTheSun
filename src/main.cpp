#include "core.h"
#include "TEMPORARY_NEEDS_CLEANUP/imgui/imgui_impl_glfw.h"
#include "TEMPORARY_NEEDS_CLEANUP/imgui/imgui_impl_opengl3.h"
#include "TEMPORARY_NEEDS_CLEANUP/glfw/glfw3.h"

STRUCT(Player)
{
	Vector2 pos;
};

Texture *background;
Font roboto;
Player player;

void Game(void)
{
	InitWindow(1280, 720, "Who Stole The Sun");
	SetTargetFPS(60);
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
	ImGui_ImplOpenGL3_Init("#version 130");

	roboto = LoadFontAscii("res/Roboto.ttf", 32);
	background = LoadTextureAndTrackChanges("res/background.png");
	
	player.pos.x = 1280 / 2;
	player.pos.y = 720 / 2;

	float theta = 22.5f * DEG2RAD;
	float ySquish = sqrtf(2) * sinf(theta);

	while (not WindowShouldClose())
	{
		HotReloadAllTrackedItems();
		TempReset(0);

		BeginDrawing();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		{
			ClearBackground(BLACK); // @TODO: We might not need to clear if we always overwrite the whole screen.
			DrawTexture(*background, 0, 0, WHITE);

			float moveSpeed = 5;
			Vector2 move = { 0 };
			if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
				move.x -= 1;
			if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
				move.x += 1;
			if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
				move.y -= 1;
			if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
				move.y += 1;
			if (move.x != 0 || move.y != 0)
			{
				move = Vector2Normalize(move);
				Vector2 deltaPos = Vector2Scale(move, moveSpeed);
				deltaPos.y *= ySquish;
				player.pos = Vector2Add(player.pos, deltaPos);
			}

			Vector2 playerSize = { 50, 90 };
			DrawRectangleV(player.pos, playerSize, RED);

			if (IsKeyDown(KEY_GRAVE))
				ImGui::ShowDemoWindow();
		}			
		rlDrawRenderBatchActive();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		EndDrawing();
	}
}

void GameLoopOneIteration(void)
{

}
