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

STRUCT(Object)
{
	const char *name;
	Vector2 position;
	Image *collisionMap;
	Direction direction;
	List(Texture *) sprites[DIRECTION_ENUM_COUNT];
	int animationFrame;
	Script *script;
	int numExpressions;
	Expression expressions[10]; // We might want more, but this should generally be a very small number.

	void Update()
	{
		if (ListCount(sprites[direction]) == 0)
			return;

		List(Texture *) sprite = sprites[direction];
		animationFrame = (animationFrame + 1) % ListCount(sprite);
	}

	void Render()
	{
		if (ListCount(sprites[direction]) == 0)
			return;

		List(Texture *) sprite = sprites[direction];
		DrawTextureCentered(*sprite[animationFrame], position, WHITE);
	}
};

bool devMode = true; // @TODO @SHIP: Disable this for release.
Input input;
Font roboto;
Font robotoBold;
Font robotoItalic;
Font robotoBoldItalic;
Object player;
Object objects[100];
int numObjects;
Camera2D camera;
Sound shatter;

List(Texture *) LoadAllTexturesFromDirectory(const char *path)
{
	List(Texture *) textures = NULL;
	FilePathList files = LoadDirectoryFiles(path);
	{
		for (unsigned int i = 0; i < files.count; ++i)
			ListAdd(&textures, LoadTextureAndTrackChanges(files.paths[i]));
	}
	UnloadDirectoryFiles(files);
	return textures;
}
bool CheckCollisionMap(Image map, Vector2 position)
{
	int xi = (int)floorf(position.x);
	int yi = (int)floorf(position.y);

	if (xi < 0)
		return false;
	if (xi >= map.width)
		return false;
	if (yi < 0)
		return false;
	if (yi >= map.height)
		return false;

	Color color = GetImageColor(map, xi, yi);
	return color.r < 128;
}
Vector2 MovePointWithCollisions(Vector2 position, Vector2 velocity)
{
	Vector2 p0 = position;
	Vector2 p1 = position + velocity;
	
	Vector2 newPosition = position + velocity;
	for (int i = 0; i < numObjects; ++i)
	{
		Object *object = &objects[i];
		if (object->collisionMap)
		{
			Rectangle rectangle = {
				object->position.x - 0.5f * object->collisionMap->width,
				object->position.y - 0.5f * object->collisionMap->height,
				(float)object->collisionMap->width,
				(float)object->collisionMap->height,
			};
			Vector2 topLeft = { rectangle.x, rectangle.y };
			Vector2 localPosition = newPosition - topLeft;

			// The CheckCollisionMap call might become more expensive in the future, so we first check
			// the rectangle to make sure a collision can happen at all, and only then do we actually check the collision map.
			if (CheckCollisionPointRec(newPosition, rectangle) and CheckCollisionMap(*object->collisionMap, localPosition))
				return position;
		}
	}

	return newPosition;
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
Texture *GetCurrentTexture(Object *object)
{
	if (ListCount(object->sprites[object->direction]) == 0)
		return NULL;

	return object->sprites[object->direction][object->animationFrame];
}
float PlayerDistanceToObject(Object *object)
{
	Vector2 playerFeet = player.position;
	playerFeet.y += GetCurrentTexture(&player)->height * 0.5f;
	playerFeet.y *= Y_SQUISH;

	Vector2 objectPosition = object->position;
	Texture *objectTexture = GetCurrentTexture(object);
	if (objectTexture)
	{
		objectPosition.y += objectTexture->height * 0.5f;
		objectPosition.y *= Y_SQUISH;
	}

	return Vector2Distance(playerFeet, objectPosition);
}

void CenterCameraOnPlayer()
{
	camera.target = player.position;
	camera.offset.x = WINDOW_CENTER_X;
	camera.offset.y = WINDOW_CENTER_Y;
	camera.zoom = 1;
}
void ZoomCameraToScreenPoint(Vector2 screenPoint, float zoom)
{
	Vector2 preZoom = GetScreenToWorld2D(screenPoint, camera);
	camera.zoom *= zoom;
	Vector2 postZoom = GetScreenToWorld2D(screenPoint, camera);
	Vector2 change = postZoom - preZoom;
	camera.target.x -= change.x;
	camera.target.y -= change.y;
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
		feetPos.y += 0.5f * GetCurrentTexture(&player)->height;
		Vector2 newFeetPos = MovePointWithCollisions(feetPos, deltaPos);
		player.position = player.position + (newFeetPos - feetPos);;
	}

	player.Update();
	for (int i = 0; i < numObjects; i++)
		objects[i].Update();

	CenterCameraOnPlayer();
}
void Playing_Render()
{
	ClearBackground(BLACK);
	
	BeginMode2D(camera);
	{
		for (int i = 0; i < numObjects; ++i)
		{
			Object *object = &objects[i];
			object->Render();
		}
		player.Render();
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

	RenderConsole();

	ImGui::ShowDemoWindow();
	ImGui::Begin("Objects");
	{
		
	}
	ImGui::End();
	
	if (not ImGui::GetIO().WantCaptureMouse)
	{
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
		{
			SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
			Vector2 delta = GetMouseDelta();
			camera.target.x -= delta.x / camera.zoom;
			camera.target.y -= delta.y / camera.zoom;
		}
		else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

		float wheel = GetMouseWheelMove();
		if (wheel > 0)
			ZoomCameraToScreenPoint(GetMousePosition(), 1.1f);
		else if (wheel < 0)
			ZoomCameraToScreenPoint(GetMousePosition(), 1 / 1.1f);

		if (IsKeyPressed(KEY_C))
			CenterCameraOnPlayer();
	}
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
	//ToggleFullscreen();

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
	shatter = LoadSound("res/shatter.wav");
	
	player.name = "Player";
	player.position.x = 1280 / 2;
	player.position.y = 720 / 2;
	player.direction = DIRECTION_DOWN;
	player.sprites[DIRECTION_RIGHT     ] = LoadAllTexturesFromDirectory("res/player_right/");
	player.sprites[DIRECTION_UP_RIGHT  ] = LoadAllTexturesFromDirectory("res/player_up_right/");
	player.sprites[DIRECTION_UP        ] = LoadAllTexturesFromDirectory("res/player_up/");
	player.sprites[DIRECTION_UP_LEFT   ] = LoadAllTexturesFromDirectory("res/player_up_left/");
	player.sprites[DIRECTION_LEFT      ] = LoadAllTexturesFromDirectory("res/player_left/");
	player.sprites[DIRECTION_DOWN_LEFT ] = LoadAllTexturesFromDirectory("res/player_down_left/");
	player.sprites[DIRECTION_DOWN      ] = LoadAllTexturesFromDirectory("res/player_down/");
	player.sprites[DIRECTION_DOWN_RIGHT] = LoadAllTexturesFromDirectory("res/player_down_right/");
	player.expressions[0].portrait = LoadTextureAndTrackChanges("res/player-neutral.png");
	CopyString(player.expressions[0].name, "neutral", sizeof player.expressions[0].name);
	player.numExpressions = 1;

	Object *background = &objects[numObjects++];
	background->name = "Background";
	ListAdd(&background->sprites[0], LoadTextureAndTrackChanges("res/background.png"));
	background->collisionMap = LoadImageAndTrackChangesEx("res/collision-map.png", PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
	background->position.x = 0.5f * background->sprites[0][0]->width;
	background->position.y = 0.5f * background->sprites[0][0]->height;

	Object *pinkGuy = &objects[numObjects++];
	pinkGuy->name = "Pink guy";
	ListAdd(&pinkGuy->sprites[0], LoadTextureAndTrackChanges("res/pink-guy.png"));
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
	ListAdd(&greenGuy->sprites[0], LoadTextureAndTrackChanges("res/green-guy.png"));
	greenGuy->position.x = 1000;
	greenGuy->position.y = 250;
	greenGuy->script = LoadScriptAndTrackChanges("res/green-guy-script.txt", roboto, robotoBold, robotoItalic, robotoBoldItalic);
	greenGuy->expressions[0].portrait = LoadTextureAndTrackChanges("res/green-guy-neutral.png");
	CopyString(greenGuy->expressions[0].name, "neutral", sizeof greenGuy->expressions[0].name);
	greenGuy->numExpressions = 1;

	//Object *cauldron = &objects[numObjects++];
	//cauldron->name = "Cauldron";
	//cauldron->position.x = 1000;
	//cauldron->position.y = 550;
	//cauldron->spriteMgr.AddSprite("res/cauldron/");

	AddCommand("tp", &HandlePlayerTeleportCommand, "");
	AddCommand("dev", &HandleToggleDevModeCommand, "");

	SetCurrentGameState(GAMESTATE_PLAYING, NULL);
}
