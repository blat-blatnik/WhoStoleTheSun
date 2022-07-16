#include "../core.h"
#include "../lib/imgui/imgui_impl_raylib.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// Run on a dedicated GPU if both a dedicated and integrated one are avaliable.
// See: https://stackoverflow.com/a/39047129
#ifdef _MSC_VER
extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

static void DoOneFrame()
{
	UpdateAllChangedAssets();
	TempReset(0);
	BeginDrawing();
	UpdateInputMappings();
	ImGui_ImplRaylib_NewFrame();
	ImGui::NewFrame();
	{
		UpdateCurrentGameState();
		RenderCurrentGameState();
	}
	rlDrawRenderBatchActive();
	ImGui::Render();
	ImGui_ImplRaylib_Render(ImGui::GetDrawData());
	EndDrawing();
}

int main()
{
	// We need the 'res' folder to be accessible from the working directory before we do anything. 
	// But, on desktop we have no clue where the working directory or the app will be when we run. 
	// I mean, we do actually know, but it's different on mac/windows/linux, and I don't want to
	// hardcode it because it might change. So we just programmatically find it at runtime.
	// On web builds, we know for sure that 'res' is in the working directory, so we don't have
	// to do this dance and we can save a few instructions and startup time.
	#ifndef __EMSCRIPTEN__
	{
		ChangeDirectory(GetApplicationDirectory());
		while (not DirectoryExists("res"))
		{
			char *newDir = TempFormat("%s/..", GetWorkingDirectory());
			ReplaceChar(newDir, '\\', '/'); // This isn't necessary - but on windows it makes the log message look nicer :)
			LogWarning("Couldn't find 'res' in the working directory. Switching working directory to '%s'.", newDir);
			ChangeDirectory(newDir);
			TempFree(newDir);
		}
	}
	#endif

	GameInit();
	rlDisableBackfaceCulling(); // It's a 2D game we don't need this..
	rlDisableDepthTest();
	SetExitKey(0);
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplRaylib_Init();
	auto &io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("res/Roboto.ttf", 18);
	ImGui_ImplRaylib_LoadDefaultFontAtlas();

	// On the web, the browser wants to drive the main loop. On other platforms, we drive it.
	// See: https://emscripten.org/docs/porting/emscripten-runtime-environment.html#browser-main-loop
	#ifdef __EMSCRIPTEN__
	{
		emscripten_set_main_loop(DoOneFrame, 0, 1);
	}
	#else
	{
		while (not WindowShouldClose())
			DoOneFrame();
	}
	#endif
}


// On Windows, when running without the command line, the program entry point is WinMain instead of main.
#ifdef _WIN32
int __stdcall WinMain(void *instance, void *prevInstance, char *cmdLine, int showCmd)
{
	UNUSED(instance); UNUSED(prevInstance); UNUSED(cmdLine); UNUSED(showCmd);
	return main();
}
#endif