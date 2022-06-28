#define RAYGUI_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4244 4189 4100 4456) // rayhui.h warnings...
#include "../core.h"
#pragma warning(pop)

// Run on a dedicated GPU if both a dedicated and integrated one are avaliable.
// See: https://stackoverflow.com/a/39047129
#ifdef _MSC_VER
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

void DoOneGameLoop(void)
{
	TempReset(0);
	GameLoopOneIteration();
}

int main(void)
{
	GameInit();

	// On the web, the browser wants to drive the main loop. On other platforms, we drive it.
	// See: https://emscripten.org/docs/porting/emscripten-runtime-environment.html#browser-main-loop
	#ifdef __EMSCRIPTEN__
	{
		void emscripten_set_main_loop(void(*callback)(void), int fps, int simulate_infinite_loop);
		emscripten_set_main_loop(DoOneGameLoop, 0, 1);
	}
	#else
	{
		while (!WindowShouldClose())
			DoOneGameLoop();
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