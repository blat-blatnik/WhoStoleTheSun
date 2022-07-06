#include "core.h"
#include "lib/imgui/imgui.h"

#define FPS 60
#define DELTA_TIME (1.0f/FPS)
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define Y_SQUISH 0.541196100146197f // sqrt(2) * sin(PI / 8)

ENUM(GameState)
{
	GAMESTATE_PLAYING,
	GAMESTATE_PAUSED,
	GAMESTATE_EDITOR,
	GAMESTATE_ENUM_COUNT
};

STRUCT(Player)
{
	Vector2 position;
	Direction direction; // Determines which sprite to use.
	Texture *textures[DIRECTION_ENUM_COUNT];
};

STRUCT(Npc)
{
	Vector2 position;
	Texture *texture;
};

int frameNumber;
GameState gameState = GAMESTATE_PLAYING;
Texture *background;
Image *collisionMap;
Font roboto;
Player player;
Npc pinkGuy;
Sound shatter;

float PlayerDistanceToNpc(Npc npc)
{
	Vector2 playerFeet = player.position;
	playerFeet.y += player.textures[player.direction]->height * 0.5f;
	playerFeet.y *= Y_SQUISH;

	Vector2 npcFeet = npc.position;
	npcFeet.y += npc.texture->height * 0.5f;
	npcFeet.y *= Y_SQUISH;

	return Vector2Distance(playerFeet, npcFeet);
}

void Playing_Init(GameState oldState)
{

}
void Playing_Update()
{
	if (IsKeyPressed(KEY_GRAVE))
	{
		gameState = GAMESTATE_EDITOR;
		return;
	}
	if (IsKeyPressed(KEY_ESCAPE))
	{
		gameState = GAMESTATE_PAUSED;
		return;
	}

	if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_E))
	{
		float distance = PlayerDistanceToNpc(pinkGuy);
		LogInfo("Distance to pink guy: %g", distance);
		if (distance < 50)
			PlaySound(shatter);
	}

	float moveSpeed = 5;
	if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
		moveSpeed = 10;

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
		Vector2 dirVector = move;
		dirVector.y *= -1;
		player.direction = DirectionFromVector(dirVector);
		Vector2 deltaPos = Vector2Scale(move, moveSpeed);

		// In the isometric perspective, the y direction is squished down a little bit.
		deltaPos.y *= Y_SQUISH;
		Vector2 newPos = Vector2Add(player.position, deltaPos);
		Vector2 feetPos = newPos;
		feetPos.y += 0.5f * player.textures[player.direction]->height;

		int footX = (int)roundf(feetPos.x);
		int footY = (int)roundf(feetPos.y);
		footX = ClampInt(footX, 0, collisionMap->width - 1);
		footY = ClampInt(footY, 0, collisionMap->height - 1);
		Color collision = GetImageColor(*collisionMap, footX, footY);

		if (collision.r >= 128)
			player.position = newPos;
	}
}
void Playing_Render()
{
	ClearBackground(BLACK);
	DrawTexture(*background, 0, 0, WHITE);
	DrawTextureCentered(*player.textures[player.direction], player.position, WHITE);
	DrawTextureCentered(*pinkGuy.texture, pinkGuy.position, WHITE);
}

void Editor_Update()
{
	if (IsKeyPressed(KEY_GRAVE))
	{
		gameState = GAMESTATE_PLAYING;
		return;
	}

	ImGui::ShowDemoWindow();
}
void Editor_Render()
{
	Playing_Render();
}

void Paused_Update(void)
{
	if (IsKeyPressed(KEY_ESCAPE))
	{
		gameState = GAMESTATE_PLAYING;
		return;
	}
}
void Paused_Render(void)
{
	Playing_Render();
	DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GrayscaleAlpha(0, 0.4f));
	DrawFormatCentered(roboto, 0.5f * WINDOW_WIDTH, 0.5f * WINDOW_HEIGHT, 64, BLACK, "Paused");
}

void GameInit(void)
{
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Who Stole The Sun");
	InitAudioDevice();
	SetTargetFPS(FPS);
	
	roboto = LoadFontAscii("res/Roboto.ttf", 32);
	background = LoadTextureAndTrackChanges("res/background.png");
	collisionMap = LoadImageAndTrackChanges("res/collision-map.png");
	shatter = LoadSound("res/shatter.wav");
	
	player.position.x = 1280 / 2;
	player.position.y = 720 / 2;
	player.direction = DIRECTION_DOWN;
	player.textures[DIRECTION_RIGHT]      = LoadTextureAndTrackChanges("res/player-right.png");
	player.textures[DIRECTION_UP_RIGHT]   = LoadTextureAndTrackChanges("res/player-up-right.png");
	player.textures[DIRECTION_UP]         = LoadTextureAndTrackChanges("res/player-up.png");
	player.textures[DIRECTION_UP_LEFT]    = LoadTextureAndTrackChanges("res/player-up-left.png");
	player.textures[DIRECTION_LEFT]       = LoadTextureAndTrackChanges("res/player-left.png");
	player.textures[DIRECTION_DOWN_LEFT]  = LoadTextureAndTrackChanges("res/player-down-left.png");
	player.textures[DIRECTION_DOWN]       = LoadTextureAndTrackChanges("res/player-down.png");
	player.textures[DIRECTION_DOWN_RIGHT] = LoadTextureAndTrackChanges("res/player-down-right.png");
	
	pinkGuy.texture = LoadTextureAndTrackChanges("res/pink-guy.png");
	pinkGuy.position.x = 400;
	pinkGuy.position.y = 250;
}
void GameLoopOneIteration(void)
{
	STRUCT(GameStateFuncs)
	{
		// init and deinit can be NULL, in which case they just won't be called.
		void(*init)(GameState oldState);
		void(*deinit)();
		void(*update)();
		void(*render)();
	};

	static GameStateFuncs funcs[GAMESTATE_ENUM_COUNT];

	if (frameNumber == 0)
	{
		funcs[GAMESTATE_PLAYING] = { Playing_Init, NULL, Playing_Update, Playing_Render };
		funcs[GAMESTATE_EDITOR] = { NULL, NULL, Editor_Update, Editor_Render };
		funcs[GAMESTATE_PAUSED] = { NULL, NULL, Paused_Update, Paused_Render };

		if (funcs[gameState].init)
			funcs[gameState].init(gameState);
	}

	GameState stateBeforeUpdate = gameState;
	funcs[gameState].update();
	if (gameState != stateBeforeUpdate)
	{
		if (funcs[stateBeforeUpdate].deinit)
			funcs[stateBeforeUpdate].deinit();
		if (funcs[gameState].init)
			funcs[gameState].init(stateBeforeUpdate);
	}
	funcs[gameState].render();
	++frameNumber;
}
