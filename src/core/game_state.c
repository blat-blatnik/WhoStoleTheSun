#include "../core.h"

STRUCT(Functions)
{
	void(*init)(void *parameter);
	void(*deinit)(void);
	void(*update)(void);
	void(*render)(void);
};

STRUCT(Entry)
{
	int state;
	int frameNumber;
};

static Functions registry[100];
static int cursor;
static Entry stack[100];
static Entry current;

void RegisterGameState(int state, void(*init)(void *parameter), void(*deinit)(void), void(*update)(void), void(*render)(void))
{
	ASSERT(state >= 0 && state < COUNTOF(registry));
	registry[state].init = init;
	registry[state].deinit = deinit;
	registry[state].update = update;
	registry[state].render = render;
}

void PushGameState(int state, void *parameter)
{
	ASSERT(cursor < COUNTOF(stack)); // Stack overflow.
	ASSERT(state >= 0 && state < COUNTOF(registry));

	stack[cursor++] = current;
	current = (Entry){ state };
	if (registry[state].init)
		registry[state].init(parameter);
}

void PopGameState(void)
{
	ASSERT(cursor > 0);

	if (registry[current.state].deinit)
		registry[current.state].deinit();
	current = stack[--cursor];
}

void PopGameStateUntil(int state)
{
	ASSERT(state >= 0 && state < COUNTOF(registry));

	while (current.state != state && cursor > 0)
		PopGameState();

	ASSERT(current.state == state); // The target game state was not found in the stack.
}

void SetCurrentGameState(int state, void *parameter)
{
	ASSERT(state >= 0 && state < COUNTOF(registry));

	if (registry[current.state].deinit)
		registry[current.state].deinit();
	current.state = state;
	if (registry[current.state].init)
		registry[current.state].init(parameter);
}

void UpdateCurrentGameState(void)
{
	if (registry[current.state].update)
		registry[current.state].update();
	++current.frameNumber;
}

void RenderCurrentGameState(void)
{
	if (registry[current.state].render)
		registry[current.state].render();
}

void CallPreviousGameStateRender(void)
{
	if (cursor == 0)
		return;

	Entry previous = stack[cursor - 1];
	if (registry[previous.state].render)
	{
		// Set up the conditions as if the previous game state was current.
		--cursor;
		Entry backup = current;
		current = previous;
		{
			registry[previous.state].render();
		}
		current = backup;
		++cursor;
	}
}

int GetCurrentGameState(void)
{
	return current.state;
}

int GetPreviousGameState(void)
{
	if (cursor == 0)
		return -1;
	return stack[cursor - 1].state;
}

int GetFrameNumberInCurrentGameState(void)
{
	return current.frameNumber;
}

double GetTimeInCurrentGameState(void)
{
	return current.frameNumber * FRAME_TIME;
}

void SetFrameNumberInCurrentGameState(int frameNumber)
{
	current.frameNumber = frameNumber;
}