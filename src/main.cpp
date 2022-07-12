#include "core.h"
#include "lib/imgui/imgui.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_CENTER_X (0.5f*WINDOW_WIDTH)
#define WINDOW_CENTER_Y (0.5f*WINDOW_HEIGHT)
#define Y_SQUISH 0.5f // sqrt(2) * sin(PI / 8)
//#define Y_SQUISH 0.541196100146197f // sqrt(2) * sin(PI / 8)

ENUM(GameState)
{
	GAMESTATE_PLAYING,
	GAMESTATE_TALKING,
	GAMESTATE_PAUSED,
	GAMESTATE_EDITOR,
};

STRUCT(Expression)
{
	char name[32];
	Texture *portrait;
};

STRUCT(Input)
{
	InputAxis movement;
	InputButton interact;
	InputButton sprint;
	InputButton pause;
	InputButton console;
};

STRUCT(Sprite)
{
public:
	Sprite(const char* path)
	{
		auto files = LoadDirectoryFiles(path);
		for (unsigned int i = 0; i < files.count; i++)
		{
			auto tex = LoadTextureAndTrackChanges(files.paths[i]);
			ListAdd(&textures, tex);
		}
	}

	List(Texture*) textures = NULL;
};

STRUCT(SpriteManager)
{
public:

	SpriteManager()
	{
		_animationLength = 0;
		_animationTime = 0;
	}

	SpriteManager(int reservedSprites)
	{
		_animationLength = 0;
		_animationTime = 0;
		ListAllocate(&sprites, reservedSprites);
	}

	~SpriteManager()
	{

	}

	void Update()
	{
		// say we first want to render sprite 1
		if (!sprites)
			return;

		if (!sprites[_spriteIndex].textures)
			return;

		if (ListCount(sprites[_spriteIndex].textures) == 1)
		{
			_index = 0;
			return;
		}

		_animationLength = ListCount(sprites[_spriteIndex].textures);

		_index = int(_animationTime) % _animationLength;


		_animationTime += _speed * FRAME_TIME;

		if (_animationTime >= _animationLength)
			_animationTime = 0;
	}

	void Render(Vector2 position, float scale)
	{
		if (!&sprites[_spriteIndex])
			return;

		DrawTextureCenteredScaled(*sprites[_spriteIndex].textures[_index], position, scale, WHITE);
	}

	void AddSprite(const char* path)
	{
		if (!DirectoryExists(path))
			return;

		Sprite spr = Sprite(path);
		ListAdd(&sprites, spr);
	}
	void AddSprite(Sprite spr)
	{
		ListAdd(&sprites, spr);
	}

	void SetSprite(Sprite spr, int index)
	{
		sprites[index] = spr;
	}

	void SetAnimation(int spriteInd)
	{
		_spriteIndex = spriteInd;
	}

	void SetSpeed(float speed) { _speed = speed; }
private:

	int _spriteIndex = 0;
	int _index = 0;
	float _animationTime = 0;
	int _animationLength = 0;
	float _speed = 1;

	List(Sprite) sprites = NULL;

};

STRUCT(Object)
{
	bool isNpc : 1;
	const char *name;
	Vector2 position;
	Direction direction;
	Texture *textures[DIRECTION_ENUM_COUNT];
	Image *collisionMap;
	Script *script;
	int numExpressions;
	Expression expressions[10]; // We might want more, but this should generally be a very small number.
	SpriteManager spriteMgr;
	float scale = 1;
	void Update()
	{
		spriteMgr.Update();
	}
	void Render()
	{
		spriteMgr.Render(position, scale);
	}
};

bool devMode = true; // @TODO @SHIP: Disable this for release.
Input input;
Texture *background;
Image *collisionMap;
Font roboto;
Font robotoBold;
Font robotoItalic;
Font robotoBoldItalic;
Object player;
Object objects[100];
int numObjects;
Sound shatter;

float PlayerDistanceToObject(Object *object) 
{
	Vector2 playerFeet = player.position;
	playerFeet.y += player.textures[player.direction]->height * 0.5f;
	playerFeet.y *= Y_SQUISH;

	Vector2 npcFeet = object->position;
	npcFeet.y += object->textures[0]->height * 0.5f;
	npcFeet.y *= Y_SQUISH;

	return Vector2Distance(playerFeet, npcFeet);
}
bool CheckCollisionMap(Image map, float x, float y)
{
	int xi = (int)floorf(x);
	int yi = (int)floorf(y);

	if (xi < 0)
		return true;
	if (xi >= map.width)
		return true;
	if (yi < 0)
		return true;
	if (yi >= map.height)
		return true;

	Color color = GetImageColor(map, xi, yi);
	return color.r < 128;
}
Vector2 MovePointWithCollisions(Vector2 position, Vector2 velocity)
{
	Vector2 p0 = position;
	Vector2 p1 = position + velocity;
	
	Vector2 newPosition = position + velocity;
	if (not CheckCollisionMap(*collisionMap, newPosition.x, newPosition.y))
		return newPosition;
	else
		return position;
}

Object *FindObjectByName(const char *name)
{
	if (StringsEqualNocase(name, "player"))
		return &player;
	
	for (int i = 0; i < numObjects; ++i)
		if (StringsEqualNocase(objects[i].name, name))
			return &objects[i];

	return NULL;
}
Texture GetCharacterPortrait(Object *object, const char *name)
{
	for (int i = 0; i < object->numExpressions; ++i)
		if (StringsEqualNocase(object->expressions[i].name, name))
			return *object->expressions[i].portrait;
	return *object->expressions[0].portrait;
}

// Console commands.

bool HandlePlayerTeleportCommand(List(const char *) args)
{
	// move x y
	if (ListCount(args) < 2)
		return false;

	int x = strtoul(args[0], NULL, 10);
	int y = strtoul(args[1], NULL, 10);

	player.position.x = (float)x;
	player.position.y = (float)y;

	return true;
}
bool HandleToggleDevModeCommand(List(const char *) args)
{
	if (ListCount(args) == 0)
	{
		devMode = not devMode;
		return true;
	}

	bool success;
	bool arg = ParseCommandBoolArg(args[0], &success);
	if (!success)
		return false;

	devMode = arg;
	return true;
}

//
// Playing
//

void Playing_Update()
{
	if (input.console.wasPressed)
	{
		PushGameState(GAMESTATE_EDITOR, NULL);
		return;
	}
	if (input.pause.wasPressed)
	{
		PushGameState(GAMESTATE_PAUSED, NULL);
		return;
	}

	if (input.interact.wasPressed)
	{
		for (int i = 0; i < numObjects; ++i)
		{
			Object *object = &objects[i];
			if (not object->script)
				continue;
			if (PlayerDistanceToObject(object) < 50)
			{
				PlaySound(shatter); // @TODO Remove
				PushGameState(GAMESTATE_TALKING, object);
				return;
			}
		}
	}

	float moveSpeed = 5;
	if (input.sprint.isDown)
		moveSpeed = 10;

	Vector2 move = input.movement.position;
	float magnitude = Vector2Length(move);
	if (magnitude > 0.2f)
	{
		move = Vector2Normalize(move);
		magnitude = Clamp01(Remap(magnitude, 0.2f, 0.8f, 0, 1));
		move.x *= magnitude;
		move.y *= magnitude * Y_SQUISH;

		Vector2 dirVector = move;
		dirVector.y *= -1;
		player.direction = DirectionFromVector(dirVector);
		Vector2 deltaPos = Vector2Scale(move, moveSpeed);

		// In the isometric perspective, the y direction is squished down a little bit.
		Vector2 feetPos = player.position;
		feetPos.y += 0.5f * player.textures[player.direction]->height;
		Vector2 newFeetPos = MovePointWithCollisions(feetPos, deltaPos);
		player.position = player.position + (newFeetPos - feetPos);
		player.spriteMgr.SetAnimation(player.direction);
	}

	player.Update();
	for (int i = 0; i < numObjects; i++)
		objects[i].Update();
}
void Playing_Render()
{
	ClearBackground(BLACK);
	
	Camera2D camera = { 0 };
	camera.target = player.position;
	camera.offset.x = WINDOW_CENTER_X;
	camera.offset.y = WINDOW_CENTER_Y;
	camera.zoom = 1;
	BeginMode2D(camera);
	{
		DrawTexture(*background, 0, 0, WHITE);
		for (int i = 0; i < numObjects; ++i)
		{
			Object *object = &objects[i];
			DrawTextureCentered(*object->textures[0], object->position, WHITE);
		}
		player.Render();
		//DrawTextureCentered(*player.textures[player.direction], player.position, WHITE);
	}
	EndMode2D();
}
REGISTER_GAME_STATE(GAMESTATE_PLAYING, NULL, NULL, Playing_Update, Playing_Render);

//
// Talking
//

Object *talkingObject;
int paragraphIndex;

void Talking_Init(void *param)
{
	talkingObject = (Object *)param;
	talkingObject->script->commandIndex = 0;
	paragraphIndex = 0;
}
void Talking_Update()
{
	if (input.pause.wasPressed)
	{
		PushGameState(GAMESTATE_PAUSED, NULL);
		return;
	}

	Script *script = talkingObject->script;
	int numParagraphs = ListCount(script->paragraphs);
	if (paragraphIndex >= numParagraphs)
		paragraphIndex = numParagraphs - 1;

	if (devMode and IsKeyPressed(KEY_LEFT))
	{
		paragraphIndex = ClampInt(paragraphIndex - 1, 0, numParagraphs - 1);
		SetFrameNumberInCurrentGameState(0);
	}
	if (devMode and IsKeyPressed(KEY_RIGHT))
	{
		if (paragraphIndex == numParagraphs - 1)
			SetFrameNumberInCurrentGameState(99999); // Should be enough to skip over to the end of the dialog.
		else
		{
			paragraphIndex = ClampInt(paragraphIndex + 1, 0, numParagraphs - 1);
			SetFrameNumberInCurrentGameState(0);
		}
	}

	if (input.interact.wasPressed)
	{
		float t = (float)GetTimeInCurrentGameState();
		float paragraphDuration = script->paragraphs[paragraphIndex].duration;
		if (20 * t < paragraphDuration)
		{
			SetFrameNumberInCurrentGameState(99999); // Should be enough to skip over to the end of the dialog.
		}
		else
		{
			++paragraphIndex;
			if (paragraphIndex >= ListCount(script->paragraphs))
			{
				PopGameState();
				return;
			}
			else SetFrameNumberInCurrentGameState(0);
		}
	}
}
void Talking_Render()
{
	CallPreviousGameStateRender();

	Script *script = talkingObject->script;
	Paragraph paragraph = script->paragraphs[paragraphIndex];
	const char *speaker = paragraph.speaker;
	if (!speaker)
		speaker = talkingObject->name;

	float time = 20 * (float)GetTimeInCurrentGameState();
	const char *expression = GetScriptExpression(*script, paragraphIndex, time);

	Rectangle textbox = {
		WINDOW_CENTER_X - 300,
		WINDOW_HEIGHT - 340,
		600,
		320
	};
	Rectangle portraitBox = textbox;
	portraitBox.x = 30;
	portraitBox.width = 300;

	// Portrait
	{
		Rectangle indented = ExpandRectangle(portraitBox, -5);
		Rectangle textArea = ExpandRectangle(portraitBox, -15);
		Rectangle dropShadow = { portraitBox.x + 10, portraitBox.y + 10, portraitBox.width, portraitBox.height };

		DrawRectangleRounded(dropShadow, 0.1f, 5, BLACK);
		DrawRectangleRounded(portraitBox, 0.1f, 5, WHITE);
		DrawRectangleRounded(indented, 0.1f, 5, Darken(WHITE, 2));

		Object *speakerObject = FindObjectByName(speaker);
		if (speakerObject)
		{
			Texture portrait = GetCharacterPortrait(speakerObject, expression);
			DrawTextureCentered(portrait, RectangleCenter(portraitBox), WHITE);
		}
	}

	// Text
	{
		Rectangle indented = ExpandRectangle(textbox, -5);
		Rectangle textArea = ExpandRectangle(textbox, -15);
		Rectangle dropShadow = { textbox.x + 10, textbox.y + 10, textbox.width, textbox.height };

		DrawRectangleRounded(dropShadow, 0.1f, 5, BLACK);
		DrawRectangleRounded(textbox, 0.1f, 5, WHITE);
		DrawRectangleRounded(indented, 0.1f, 5, Darken(WHITE, 2));

		DrawFormat(script->font, textArea.x + 2, textArea.y + 2, 32, BlendColors(RED, BLACK, 0.8f), "[%s] [%s]", speaker, expression);
		DrawFormat(script->font, textArea.x, textArea.y, 32, RED, "[%s] [%s]", speaker, expression);
		float yAdvance = 2 * GetLineHeight(script->font, 32);
		textArea = ExpandRectangleEx(textArea, -yAdvance, 0, 0, 0);

		DrawScriptParagraph(script, paragraphIndex, textArea, 32, PINK, BlendColors(PINK, BLACK, 0.8f), time);
	}
}
REGISTER_GAME_STATE(GAMESTATE_TALKING, Talking_Init, NULL, Talking_Update, Talking_Render);

//
// Editor
//

void Editor_Update()
{
	if (input.console.wasPressed)
	{
		PopGameState();
		return;
	}

	ImGui::ShowDemoWindow();
	RenderConsole();
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
	if (input.pause.wasPressed)
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

void GameInit(void)
{
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Who Stole The Sun");
	InitAudioDevice();
	SetTargetFPS(FPS);

	// Input mapping
	{
		MapKeyToInputButton(KEY_SPACE, &input.interact);
		MapKeyToInputButton(KEY_E, &input.interact);
		MapGamepadButtonToInputButton(GAMEPAD_BUTTON_RIGHT_FACE_DOWN, &input.interact);

		MapKeyToInputAxis(KEY_W, &input.movement, 0, -1);
		MapKeyToInputAxis(KEY_S, &input.movement, 0, +1);
		MapKeyToInputAxis(KEY_A, &input.movement, -1, 0);
		MapKeyToInputAxis(KEY_D, &input.movement, +1, 0);
		MapKeyToInputAxis(KEY_UP, &input.movement, 0, -1);
		MapKeyToInputAxis(KEY_DOWN, &input.movement, 0, +1);
		MapKeyToInputAxis(KEY_LEFT, &input.movement, -1, 0);
		MapKeyToInputAxis(KEY_RIGHT, &input.movement, +1, 0);
		MapGamepadButtonToInputAxis(GAMEPAD_BUTTON_LEFT_FACE_UP, &input.movement, 0, -1);
		MapGamepadButtonToInputAxis(GAMEPAD_BUTTON_LEFT_FACE_DOWN, &input.movement, 0, +1);
		MapGamepadButtonToInputAxis(GAMEPAD_BUTTON_LEFT_FACE_LEFT, &input.movement, -1, 0);
		MapGamepadButtonToInputAxis(GAMEPAD_BUTTON_LEFT_FACE_RIGHT, &input.movement, +1, 0);
		MapGamepadAxisToInputAxis(GAMEPAD_AXIS_LEFT_X, &input.movement);

		MapKeyToInputButton(KEY_LEFT_SHIFT, &input.sprint);
		MapKeyToInputButton(KEY_RIGHT_SHIFT, &input.sprint);
		MapGamepadButtonToInputButton(GAMEPAD_BUTTON_RIGHT_TRIGGER_2, &input.sprint);

		MapKeyToInputButton(KEY_ESCAPE, &input.pause);
		MapGamepadButtonToInputButton(GAMEPAD_BUTTON_MIDDLE_RIGHT, &input.pause);

		MapKeyToInputButton(KEY_GRAVE, &input.console);
	}

	roboto = LoadFontAscii("res/roboto.ttf", 32);
	robotoBold = LoadFontAscii("res/roboto-bold.ttf", 32);
	robotoItalic = LoadFontAscii("res/roboto-italic.ttf", 32);
	robotoBoldItalic = LoadFontAscii("res/roboto-bold-italic.ttf", 32);

	background = LoadTextureAndTrackChanges("res/background2.png");
	collisionMap = LoadImageAndTrackChangesEx("res/collision-map2.png", PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
	shatter = LoadSound("res/shatter.wav");
	
	player.name = "Player";
	player.position.x = 1280 / 2;
	player.position.y = 720 / 2;
	player.direction = DIRECTION_DOWN;
	player.textures[DIRECTION_RIGHT     ] = LoadTextureAndTrackChanges("res/player-right.png");
	player.textures[DIRECTION_UP_RIGHT  ] = LoadTextureAndTrackChanges("res/player-up-right.png");
	player.textures[DIRECTION_UP        ] = LoadTextureAndTrackChanges("res/player-up.png");
	player.textures[DIRECTION_UP_LEFT   ] = LoadTextureAndTrackChanges("res/player-up-left.png");
	player.textures[DIRECTION_LEFT      ] = LoadTextureAndTrackChanges("res/player-left.png");
	player.textures[DIRECTION_DOWN_LEFT ] = LoadTextureAndTrackChanges("res/player-down-left.png");
	player.textures[DIRECTION_DOWN      ] = LoadTextureAndTrackChanges("res/player-down.png");
	player.textures[DIRECTION_DOWN_RIGHT] = LoadTextureAndTrackChanges("res/player-down-right.png");
	player.expressions[0].portrait = LoadTextureAndTrackChanges("res/player-neutral.png");
	CopyString(player.expressions[0].name, "neutral", sizeof player.expressions[0].name);
	player.numExpressions = 1;
	player.spriteMgr.AddSprite("res/player_right/");
	player.spriteMgr.AddSprite("res/player_up_right/");
	player.spriteMgr.AddSprite("res/player_up/");
	player.spriteMgr.AddSprite("res/player_up_left/");
	player.spriteMgr.AddSprite("res/player_left/");
	player.spriteMgr.AddSprite("res/player_down_left/");
	player.spriteMgr.AddSprite("res/player_down/");
	player.spriteMgr.AddSprite("res/player_down_right/");

	Object *pinkGuy = &objects[numObjects++];
	pinkGuy->name = "Pink guy";
	pinkGuy->textures[0] = LoadTextureAndTrackChanges("res/pink-guy.png");
	pinkGuy->position.x = 700;
	pinkGuy->position.y = 250;
	pinkGuy->script = LoadScriptAndTrackChanges("res/example-script.txt", roboto, robotoBold, robotoItalic, robotoBoldItalic);
	pinkGuy->expressions[0].portrait = LoadTextureAndTrackChanges("res/pink-guy-neutral.png");
	pinkGuy->expressions[1].portrait = LoadTextureAndTrackChanges("res/pink-guy-happy.png");
	pinkGuy->expressions[2].portrait = LoadTextureAndTrackChanges("res/pink-guy-sad.png");
	CopyString(pinkGuy->expressions[0].name, "neutral", sizeof pinkGuy->expressions[0].name);
	CopyString(pinkGuy->expressions[1].name, "happy", sizeof pinkGuy->expressions[1].name);
	CopyString(pinkGuy->expressions[2].name, "sad", sizeof pinkGuy->expressions[2].name);
	pinkGuy->numExpressions = 3;

	Object *greenGuy = &objects[numObjects++];
	greenGuy->name = "Green guy";
	greenGuy->textures[0] = LoadTextureAndTrackChanges("res/green-guy.png");
	greenGuy->position.x = 1000;
	greenGuy->position.y = 250;
	greenGuy->script = LoadScriptAndTrackChanges("res/green-guy-script.txt", roboto, robotoBold, robotoItalic, robotoBoldItalic);
	greenGuy->expressions[0].portrait = LoadTextureAndTrackChanges("res/green-guy-neutral.png");
	CopyString(greenGuy->expressions[0].name, "neutral", sizeof greenGuy->expressions[0].name);
	greenGuy->numExpressions = 1;

	AddCommand("tp", &HandlePlayerTeleportCommand, "");
	AddCommand("dev", &HandleToggleDevModeCommand, "");
	

	SetCurrentGameState(GAMESTATE_PLAYING, NULL);
}
