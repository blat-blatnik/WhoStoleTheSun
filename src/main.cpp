

#include "core.h"
#include "text.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <stdio.h>
Texture *test;
Font roboto;


bool show_demo_window = true;
void GameInit(void)
{
	InitWindow(1280, 720, "Who Stole The Sun");
	SetTargetFPS(60);

	int ascii[128];
	for (int i = 0; i < COUNTOF(ascii); ++i)
		ascii[i] = i;
	roboto = LoadFontEx("res/Roboto.ttf", 32, ascii, COUNTOF(ascii));

	test = LoadTextureAndTrackChanges("res/test.png");


	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	GLFWwindow* window = glfwGetCurrentContext();
	if (window == NULL)
		return;
	
	//glfwMakeContextCurrent(window);
	//glfwSwapInterval(1); // Enable vsync


	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	

	while (not WindowShouldClose())
	{
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		HotReloadAllTrackedItems();
		TempReset(0);

		BeginDrawing();
		{
			ClearBackground(BLACK);

			DrawTexture(*test, 50, 50, WHITE);
			DrawFormat(roboto, 500, 300, 32, WHITE, "Hello, sailor!\nWho killed captain Alex?");

			Rectangle textBox = { 500, 100, 400, 600 };
			float t = (float)(GetTime() - 1);

			const char* text =
				"This is the story of a man named Stanley. "
				"Stanley worked for a company in a big building where he was Employee #427. "
				"Employee #427's job was simple: he sat at his desk in Room 427 and he pushed buttons on a keyboard. "
				"Orders came to him through a monitor on his desk telling him what buttons to push, how long to push them, and in what order. "
				"This is what Employee #427 did every day of every month of every year, andalthough others may have considered it soul rending, "
				"Stanley relished every moment that the orders came in, as though he had been made exactly for this job. "
				"And Stanley was happy. "
				"And then one day, something very peculiar happened. "
				"Something that would forever change Stanley; "
				"Something he would never quite forget. "
				"He had been at his desk for nearly an hour when he had realized not one single order had arrived on the monitor for him to follow. "
				"No one had shown up to give him instructions, call a meeting, or even say 'hi'.Never in all his years at the company had this happened, this complete isolation. "
				"Something was very clearly wrong.Shocked, frozen solid, Stanley found himself unable to move for the longest time. "
				"But as he came to his wits andregained his senses, he got up from his desk andstepped out of his office. ";

			DrawAnimatedTextBox(roboto, textBox, 32, BLACK, 20 * t, text);
		}

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		rlDrawRenderBatchActive();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		EndDrawing();
	}
}

void GameLoopOneIteration(void)
{
	




}