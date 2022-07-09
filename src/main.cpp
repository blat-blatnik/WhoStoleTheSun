#include "core.h"
#include "lib/imgui/imgui.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_CENTER_X (0.5f*WINDOW_WIDTH)
#define WINDOW_CENTER_Y (0.5f*WINDOW_HEIGHT)
#define Y_SQUISH 0.541196100146197f // sqrt(2) * sin(PI / 8)

ENUM(GameState)
{
	GAMESTATE_PLAYING,
	GAMESTATE_TALKING,
	GAMESTATE_PAUSED,
	GAMESTATE_EDITOR,
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

Texture *background;
Image *collisionMap;
Font roboto;
Player player;
Npc pinkGuy;
Sound shatter;

Console console;


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

//
// Playing
//

void Playing_Update()
{
	if (IsKeyPressed(KEY_GRAVE))
	{
		PushGameState(GAMESTATE_EDITOR, NULL);
		return;
	}
	if (IsKeyPressed(KEY_ESCAPE))
	{
		PushGameState(GAMESTATE_PAUSED, NULL);
		return;
	}

	if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_E))
	{
		float distance = PlayerDistanceToNpc(pinkGuy);
		LogInfo("Distance to pink guy: %g", distance);
		if (distance < 50)
		{
			PlaySound(shatter); // @TODO Remove
			PushGameState(GAMESTATE_TALKING, NULL);
			return;
		}
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
REGISTER_GAME_STATE(GAMESTATE_PLAYING, NULL, NULL, Playing_Update, Playing_Render);

//
// Talking
//

void Talking_Update()
{
	if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE))
	{
		PopGameState();
		return;
	}
	if (IsKeyPressed(KEY_ESCAPE))
	{
		PushGameState(GAMESTATE_PAUSED, NULL);
		return;
	}
}
void Talking_Render()
{
	CallPreviousGameStateRender();

	Rectangle textbox = {
		WINDOW_CENTER_X - 300,
		WINDOW_HEIGHT - 340,
		600,
		320
	};
	Rectangle indented = ExpandRectangle(textbox, -5);
	Rectangle textArea = ExpandRectangle(textbox, -15);
	Rectangle dropShadow = { textbox.x + 10, textbox.y + 10, textbox.width, textbox.height };

	DrawRectangleRounded(dropShadow, 0.1f, 5, BLACK);
	DrawRectangleRounded(textbox, 0.1f, 5, WHITE);
	DrawRectangleRounded(indented, 0.1f, 5, Darken(WHITE, 2));

	float t = (float)GetTimeInCurrentGameState();
	DrawAnimatedTextBox(roboto, textArea, 32, PINK, 25 * t, 
		"Hello, sailor!\n\n"
		"This is the story of a man named Stanley.\n\n"
		"Seven salty sailors sail the seven salty sees.\n\n"
		"Several boxing wizards jump quickly.\n\n"
		"Would you like to know more?");
}
REGISTER_GAME_STATE(GAMESTATE_TALKING, NULL, NULL, Talking_Update, Talking_Render);

//
// Editor
//

void Editor_Update()
{
	if (IsKeyPressed(KEY_GRAVE))
	{
		PopGameState();
		return;
	}

	ImGui::ShowDemoWindow();
	console.ShowConsoleWindow("Console", NULL);
}
void Editor_Render()
{
	CallPreviousGameStateRender();
}
REGISTER_GAME_STATE(GAMESTATE_EDITOR, NULL, NULL, Editor_Update, Editor_Render);

//
// Paused
//

void Paused_Update(void)
{
	if (IsKeyPressed(KEY_ESCAPE))
	{
		PopGameState();
		return;
	}
}
void Paused_Render(void)
{
	CallPreviousGameStateRender();
	DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GrayscaleAlpha(0, 0.4f));
	DrawFormatCentered(roboto, WINDOW_CENTER_X, WINDOW_CENTER_Y, 64, BLACK, "Paused");
}
REGISTER_GAME_STATE(GAMESTATE_PAUSED, NULL, NULL, Paused_Update, Paused_Render);

bool HandlePlayerTeleportCommand(std::vector<std::string> args)
{
	// move x y
	if (args.size() < 2)
		return false;

	int x = strtoul(args[0].c_str(), NULL, 10);
	int y = strtoul(args[1].c_str(), NULL, 10);

	player.position.x = x;
	player.position.y = y;

	return true;
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


	// teleport player
	console.AddCommand("tp", &HandlePlayerTeleportCommand);
	console.GetCommand("tp")->SetHelp("This needs help for sure");
	SetCurrentGameState(GAMESTATE_PLAYING, NULL);
}
