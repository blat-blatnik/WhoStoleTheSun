#include "core.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_CENTER_X (0.5f*WINDOW_WIDTH)
#define WINDOW_CENTER_Y (0.5f*WINDOW_HEIGHT)
#define MAX_SHAKE_ROTATION (5*DEG2RAD)
#define MAX_SHAKE_TRANSLATION 50.0f
#define DEFAULT_CAMERA_SHAKE_TRAUMA 0.5f
#define DEFAULT_CAMERA_SHAKE_FALLOFF (0.7f * FRAME_TIME)
#define SCENE_MAGIC "KEKW"
#define SCENE_VERSION 2 // You need to increase this every time the scene binary format changes!
#define Y_SQUISH 0.5773502691896258f // 1 / (2 * cos(30 degrees)) = 1 / sqrt(3)
#define GRID_RESOLUTION_X 50.0f
#define GRID_RESOLUTION_Y (GRID_RESOLUTION_X * Y_SQUISH)

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

STRUCT(MotionMaster)
{
	void MoveToPoint(Vector2 start, Vector2 end)
	{
		if (start == end)
			return;

		startPoint = start;
		endPoint = end;
		
		arrivalTime = Vector2Distance(startPoint, endPoint) / speed;
		isMoving = true;
	}


	void Update()
	{
		if (!isMoving)
			return;

		if (motionTime + FRAME_TIME * speed <= arrivalTime)
		{
			motionTime += FRAME_TIME * speed;
			currentPoint.x = Lerp(startPoint.x, endPoint.x, motionTime / arrivalTime);
			currentPoint.y = Lerp(startPoint.y, endPoint.y, motionTime / arrivalTime);
		}
		else
		{
			currentPoint = endPoint;
			
			Reset();
		}	
	}

	Vector2 GetDirection()
	{
		auto subtract = Vector2Subtract(endPoint, startPoint);
		subtract.y = -subtract.y;
		return Vector2Normalize(subtract);
	}

	Vector2 currentPoint;
	bool isMoving = false;

	void Reset()
	{
		endPoint = Vector2Zero();
		startPoint = Vector2Zero();
		isMoving = false;
		motionTime = 0;
		arrivalTime = 0;
		speed = 10;
	}
	Vector2 startPoint;
	Vector2 endPoint;


	float motionTime = 0;
	float arrivalTime = 0;
	float speed = 10;
};

STRUCT(Object)
{
	char name[50];
	Vector2 position;
	float zOffset;
	Direction direction;
	Sprite *sprites[DIRECTION_ENUM_COUNT];
	float animationFps;
	float animationTimeAccumulator;
	int animationFrame;
	Image *collisionMap;
	Script *script;
	Expression expressions[10]; // We might want more, but this should generally be a very small number.
	MotionMaster motionMaster;
};

STRUCT(Stair)
{
	int gridX;
	int gridY;
	int elevation;
};

bool devMode = true; // @TODO @SHIP: Disable this for release.
Input input;
Font roboto;
Font robotoBold;
Font robotoItalic;
Font robotoBoldItalic;
Object objects[100];
Object *player = &objects[0]; // The player is ALWAYS the first object.
int numObjects;
Camera2D camera;
float cameraTrauma; // Amount of camera shake. Will slowly decrease over time.
float cameraTraumaFalloff; // How quickly the camera shake stops.
float cameraOffsetFactor = 25; // How far ahead the camera goes in the direction of player movement.
float cameraAcceleration = 0.03f; // How quickly the camera converges on it's desired offset.
Vector2 cameraOffset;
int numStairs;
Stair stairs[10];

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
	for (int i = 0; i < numObjects; ++i)
		if (StringsEqualNocase(objects[i].name, name))
			return &objects[i];

	return NULL;
}
Texture GetCharacterPortrait(const Object *object, const char *name)
{
	for (int i = 0; i < COUNTOF(object->expressions); ++i)
		if (StringsEqualNocase(object->expressions[i].name, name))
			return *object->expressions[i].portrait;
	return *object->expressions[0].portrait;
}
Sprite *GetCurrentSprite(const Object *object)
{
	Sprite *sprite = object->sprites[object->direction];
	if (not sprite)
		sprite = object->sprites[MirrorDirectionVertically(object->direction)];
	return sprite;
}
Texture *GetCurrentTexture(const Object *object)
{
	Sprite *sprite = GetCurrentSprite(object);
	if (not sprite)
		return NULL;

	return &sprite->frames[object->animationFrame];
}
Vector2 GetFootPositionInScreenSpace(const Object *object)
{
	Vector2 position = object->position;

	Texture *objectTexture = GetCurrentTexture(object);
	if (objectTexture)
		position.y += objectTexture->height * 0.5f;

	return position;
}
Vector2 GetFootPositionInWorldSpace(const Object *object)
{
	Vector2 position = GetFootPositionInScreenSpace(object);
	position.y *= Y_SQUISH;
	return position;
}
float DistanceBetween(const Object *a, const Object *b)
{
	Vector2 positionA = GetFootPositionInWorldSpace(a);
	Vector2 positionB = GetFootPositionInWorldSpace(b);
	return Vector2Distance(positionA, positionB);
}
Rectangle GetOutline(const Object *object)
{
	Texture *texture = GetCurrentTexture(object);
	if (not texture)
	{
		Rectangle empty = { 0 };
		return empty;
	}

	Rectangle outline = {
		object->position.x - 0.5f * texture->width,
		object->position.y - 0.5f * texture->height,
		(float)texture->width,
		(float)texture->height,
	};
	return outline;
}
List(Object *) GetZSortedObjects(void)
{
	List(Object *) result = NULL;
	ListSetAllocator((void **)&result, TempRealloc, TempFree);
	Object **pointers = ListAllocate(&result, numObjects);
	for (int i = 0; i < numObjects; ++i)
		pointers[i] = &objects[i];

	Sort(pointers, numObjects, sizeof pointers[0], [](const void *left, const void *right)
	{
		Object *l = *(Object **)left;
		Object *r = *(Object **)right;
		float lz = GetFootPositionInScreenSpace(l).y + l->zOffset;
		float rz = GetFootPositionInScreenSpace(r).y + r->zOffset;
		if (lz > rz) return -1;
		if (lz < rz) return +1;
		return 0;
	});
	return result;
}
Object *FindObjectAtPosition(Vector2 position)
{
	Object *result = NULL;
	int mark = TempMark();
	{
		List(Object *) sorted = GetZSortedObjects();
		for (int i = 0; i < ListCount(sorted); ++i)
		{
			Object *object = sorted[i];
			Rectangle outline = GetOutline(object);
			if (CheckCollisionPointRec(position, outline))
			{
				result = object;
				break;
			}
		}
	}
	TempReset(mark);
	return result;
}

Vector2 GetMousePositionInWorld(void)
{
	return GetScreenToWorld2D(GetMousePosition(), camera);
}
Vector2 GridToWorld(Vector2 gridPoint)
{
	Vector2 worldPoint = {
		(gridPoint.x - gridPoint.y) * GRID_RESOLUTION_X / 2,
		(gridPoint.x + gridPoint.y) * GRID_RESOLUTION_Y / 2
	};
	return worldPoint;
}
Vector2 GridToScreen(Vector2 gridPoint)
{
	return GetWorldToScreen2D(GridToWorld(gridPoint), camera);
}
Vector2 WorldToGrid(Vector2 worldPoint)
{
	float worldX = 2 * worldPoint.x / GRID_RESOLUTION_X; // wx = gx - gy
	float worldY = 2 * worldPoint.y / GRID_RESOLUTION_Y; // wy = gx + gy
	Vector2 gridPoint = {
		0.5f * (worldX + worldY),
		0.5f * (worldY - worldX)
	};
	return gridPoint;
}
Vector2 ScreenToGrid(Vector2 screenPoint)
{
	return WorldToGrid(GetScreenToWorld2D(screenPoint, camera));
}
Stair *GetStairAt(Vector2 gridPoint)
{
	int x = (int)floorf(gridPoint.x);
	int y = (int)floorf(gridPoint.y);
	for (int i = 0; i < numStairs; ++i)
		if (stairs[i].gridX == x and stairs[i].gridY == y)
			return &stairs[i];
	return NULL;
}

void CenterCameraOn(Object *object)
{
	camera.target = object->position;
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
void UpdateCameraShake()
{
	cameraTrauma -= cameraTraumaFalloff;
	if (cameraTrauma <= 0)
	{
		cameraTrauma = 0;
		cameraTraumaFalloff = DEFAULT_CAMERA_SHAKE_FALLOFF;
	}
}

void Clone(Object *from, Object *to)
{
	CopyBytes(to, from, sizeof to[0]);
	to->script = (Script *)CloneAsset(from->script);
	to->collisionMap = (Image *)CloneAsset(from->collisionMap);
	for (int i = 0; i < COUNTOF(from->expressions); ++i)
		to->expressions[i].portrait = (Texture *)CloneAsset(from->expressions[i].portrait);
	for (int direction = 0; direction < DIRECTION_ENUM_COUNT; ++direction)
		to->sprites[direction] = (Sprite *)CloneAsset(from->sprites[direction]);
}
void Destroy(Object *object)
{
	ReleaseAsset(object->script);
	ReleaseAsset(object->collisionMap);
	for (int i = 0; i < COUNTOF(object->expressions); ++i)
		ReleaseAsset(object->expressions[i].portrait);
	for (int direction = 0; direction < DIRECTION_ENUM_COUNT; ++direction)
		ReleaseAsset(object->sprites[direction]);
	ZeroBytes(object, sizeof object[0]);
}
void Update(Object *object)
{
	// update sprites
	Sprite *sprite = GetCurrentSprite(object);
	if (sprite)
	{
		float animationFrameTime = 1 / object->animationFps;
		object->animationTimeAccumulator += FRAME_TIME;
		while (object->animationTimeAccumulator > animationFrameTime)
		{
			object->animationTimeAccumulator -= animationFrameTime;
			object->animationFrame = (object->animationFrame + 1) % sprite->numFrames;
		}
	}

	// update motion

	object->motionMaster.Update();
	if (object->motionMaster.isMoving)
	{
		object->position = object->motionMaster.currentPoint;
		auto dirVector = object->motionMaster.GetDirection();
		object->direction = DirectionFromVector(dirVector);
	}

}
void Render(Object *object)
{
	Sprite *sprite = GetCurrentSprite(object);
	if (not sprite)
		return;

	Vector2 position = object->position;
	Vector2 feet = GetFootPositionInScreenSpace(object);
	Stair *stair = GetStairAt(WorldToGrid(feet));
	if (stair)
		position.y -= 20 * stair->elevation;

	if (sprite == object->sprites[object->direction])
		DrawTextureCentered(sprite->frames[object->animationFrame], position, WHITE);
	else
		DrawTextureCenteredAndFlippedVertically(sprite->frames[object->animationFrame], position, WHITE);
}

char lastSavedOrLoadedScene[256];
void LoadScene(const char *path)
{
	unsigned dataSize;
	unsigned char *data = LoadFileData(path, &dataSize);
	if (not data)
	{
		if (not FileExists(path))
			LogError("Couldn't load scene from '%s' because that file doesn't exist.", path);
		else
			LogError("Couldn't load scene from '%s' because we failed to load the file contents.", path);
		return;
	}

	BinaryStream stream = { 0 };
	stream.buffer = data;
	stream.size = (int)dataSize;
	stream.cursor = 0;

	const void *magic = ReadBytes(&stream, 4);
	if (not BytesEqual(magic, SCENE_MAGIC, 4))
	{
		UnloadFileData(data);
		LogError("Couldn't load scene from '%s' because it isn't a scene file.", path);
		return;
	}

	int version = ReadInt(&stream);
	if (version != SCENE_VERSION)
	{
		UnloadFileData(data);
		LogError("Couldn't load scene from '%s' because it's version is %d, but we only handle version %d.", path, version, SCENE_VERSION);
		return;
	}

	for (int i = 0; i < numObjects; ++i)
		Destroy(&objects[i]);

	numObjects = ReadInt(&stream);
	for (int i = 0; i < numObjects; ++i)
	{
		Object *object = &objects[i];
		const char *name = ReadString(&stream);
		CopyString(object->name, name, sizeof object->name);
		
		object->position.x = ReadFloat(&stream);
		object->position.y = ReadFloat(&stream);
		object->zOffset = ReadFloat(&stream);
		object->animationFps = ReadFloat(&stream);
		object->direction = (Direction)ReadInt(&stream);
		object->script = AcquireScript(ReadString(&stream), roboto, robotoBold, robotoItalic, robotoBoldItalic);
		object->collisionMap = AcquireCollisionMap(ReadString(&stream));
		for (int dir = 0; dir < DIRECTION_ENUM_COUNT; ++dir)
			object->sprites[dir] = AcquireSprite(ReadString(&stream));
		for (int j = 0; j < COUNTOF(object->expressions); ++j)
		{
			Expression *expression = &object->expressions[j];
			const char *expressionName = ReadString(&stream);
			CopyString(expression->name, expressionName, sizeof expression->name);
			expression->portrait = AcquireTexture(ReadString(&stream));
		}
	}

	UnloadFileData(data);
	LogInfo("Successfully loaded scene '%s'.", path);
	CopyString(lastSavedOrLoadedScene, path, sizeof lastSavedOrLoadedScene);
}
void SaveScene(const char *path)
{
	int maxBytes = 32 * 1024; // 32kB should be plenty!
	void *data = TempAlloc(maxBytes);

	BinaryStream stream = { 0 };
	stream.buffer = data;
	stream.size = maxBytes;

	WriteBytes(&stream, SCENE_MAGIC, 4);
	WriteInt(&stream, SCENE_VERSION);
	WriteInt(&stream, numObjects);
	for (int i = 0; i < numObjects; ++i)
	{
		Object *object = &objects[i];
		
		WriteString(&stream, object->name);
		WriteFloat(&stream, object->position.x);
		WriteFloat(&stream, object->position.y);
		WriteFloat(&stream, object->zOffset);
		WriteFloat(&stream, object->animationFps);
		WriteInt(&stream, object->direction);
		WriteString(&stream, GetAssetPath(object->script));
		WriteString(&stream, GetAssetPath(object->collisionMap));
		for (int dir = 0; dir < DIRECTION_ENUM_COUNT; ++dir)
			WriteString(&stream, GetAssetPath(object->sprites[dir]));
		for (int j = 0; j < COUNTOF(object->expressions); ++j)
		{
			Expression *expression = &object->expressions[j];
			WriteString(&stream, expression->name);
			WriteString(&stream, GetAssetPath(expression->portrait));
		}
	}

	if (SaveFileData(path, stream.buffer, (unsigned)stream.cursor))
	{
		LogInfo("Successfully saved current scene to '%s'.", path);
		CopyString(lastSavedOrLoadedScene, path, sizeof lastSavedOrLoadedScene);
	}
	else
		LogInfo("Couldn't save current scene to '%s'.", path);
}

Vector2 SnapToGrid(Vector2 position)
{
	float ex = Wrap(position.x, 0, GRID_RESOLUTION_X / 2);
	if (ex <= 0.25f * GRID_RESOLUTION_X)
		position.x -= ex;
	else
		position.x += 0.5f * GRID_RESOLUTION_X - ex;

	float ey = Wrap(position.y, 0, GRID_RESOLUTION_Y / 2);
	if (ey <= 0.25f * GRID_RESOLUTION_Y)
		position.y -= ey;
	else
		position.y += 0.5f * GRID_RESOLUTION_Y - ey;

	return position;
}
Vector2 RoundDownToGridCell(Vector2 position)
{
	position.x -= Wrap(position.x, 0, GRID_RESOLUTION_X);
	position.y -= Wrap(position.y, 0, GRID_RESOLUTION_Y);
	return position;
}
Vector2 RoundUpToGridCell(Vector2 position)
{
	position.x += GRID_RESOLUTION_X - Wrap(position.x, 0, GRID_RESOLUTION_X);
	position.y += GRID_RESOLUTION_Y - Wrap(position.y, 0, GRID_RESOLUTION_Y);
	return position;
}

// Motion
void MoveToPoint(Object* object, Vector2 point)
{
	object->motionMaster.MoveToPoint(object->position, point);
}

// Console commands.

bool HandlePlayerTeleportCommand(List(const char *) args)
{
	// tp x y
	if (ListCount(args) < 2 or ListCount(args) > 2)
		return false;

	bool success1;
	bool success2;
	float x = ParseCommandFloatArg(args[0], &success1);
	float y = ParseCommandFloatArg(args[1], &success2);
	if (not success1 or not success2)
		return false;

	player->position.x = x;
	player->position.y = y;
	return true;
}
bool HandleToggleDevModeCommand(List(const char *) args)
{
	// dev [bool]
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
bool HandleCameraShakeCommand(List(const char *) args)
{
	// shake [trauma] [falloff]
	if (ListCount(args) > 2)
		return false;

	float trauma = DEFAULT_CAMERA_SHAKE_TRAUMA;
	float falloff = DEFAULT_CAMERA_SHAKE_FALLOFF;

	bool success1 = true;
	bool success2 = true;
	if (ListCount(args) >= 1)
		trauma = ParseCommandFloatArg(args[0], &success1);
	if (ListCount(args) >= 2)
		falloff = ParseCommandFloatArg(args[1], &success2);

	if (not success1 or not success2)
		return false;

	cameraTrauma += trauma;
	cameraTraumaFalloff = falloff;
	return true;
}
bool HandleSoundCommand(List(const char *) args)
{
	// sound filename:string [volume:float] [pitch:float]
	if (ListCount(args) > 3)
		return false;

	const char *path = args[0];
	float volume = 1;
	float pitch = 1;

	bool success1 = true;
	bool success2 = true;
	if (ListCount(args) >= 2)
		volume = ParseCommandFloatArg(args[1], &success1);
	if (ListCount(args) >= 3)
		pitch = ParseCommandFloatArg(args[2], &success2);

	if (not success1 or not success2)
		return false;

	PlayTemporarySoundEx(path, volume, pitch);

	return true;
}
bool HandleMoveBy(List(const char*) args)
{
	if (ListCount(args) != 2)
		return false;
	
	bool success1 = true;
	bool success2 = true;
	Vector2 delta = {
		ParseCommandFloatArg(args[0], &success1),
		ParseCommandFloatArg(args[1], &success2)
	};
	if (not success1 or not success2)
		return false;

	Object *object = player;
	Vector2 target = object->position + delta;
	MoveToPoint(object, target);

	return true;
}
bool HandleMoveTo(List(const char *) args)
{
	if (ListCount(args) != 2)
		return false;

	bool success1 = true;
	bool success2 = true;
	Vector2 target = {
		ParseCommandFloatArg(args[0], &success1),
		ParseCommandFloatArg(args[1], &success2)
	};
	if (not success1 or not success2)
		return false;

	Object *object = player;
	MoveToPoint(object, target);

	return true;
}
bool HandleSaveCommand(List(const char *) args)
{
	// save [filename:string]
	if (ListCount(args) > 1)
		return false;

	const char *path = lastSavedOrLoadedScene;
	if (ListCount(args) == 1)
		path = args[0];
	SaveScene(path);
	return true;
}
bool HandleLoadCommand(List(const char *) args)
{
	// load [filename:string]
	if (ListCount(args) > 1)
		return false;

	const char *path = lastSavedOrLoadedScene;
	if (ListCount(args) == 1)
		path = args[0];
	LoadScene(path);
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
			if (DistanceBetween(player, object) < 50)
			{
				PushGameState(GAMESTATE_TALKING, object);
				return;
			}
		}
	}

	float moveSpeed = 5;
	if (input.sprint.isDown)
		moveSpeed = 10;

	Vector2 playerVelocity = { 0, 0 };
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
		player->direction = DirectionFromVector(dirVector);
		Vector2 deltaPos = Vector2Scale(move, moveSpeed);
		playerVelocity = deltaPos;

		// In the isometric perspective, the y direction is squished down a little bit.
		Vector2 feetPos = player->position;
		feetPos.y += 0.5f * GetCurrentTexture(player)->height;
		Vector2 newFeetPos = MovePointWithCollisions(feetPos, deltaPos);
		player->position = player->position + (newFeetPos - feetPos);
	}

	for (int i = 0; i < numObjects; i++)
		Update(&objects[i]);

	Vector2 targetCameraOffset = cameraOffsetFactor * playerVelocity;
	cameraOffset = Vector2Lerp(cameraOffset, targetCameraOffset, cameraAcceleration);
	camera.target = player->position + cameraOffset;
	camera.offset.x = WINDOW_CENTER_X;
	camera.offset.y = WINDOW_CENTER_Y;
	camera.zoom = 1;
	UpdateCameraShake();

	ImGui::Begin("Camera");
	{
		ImGui::SliderFloat("trauma", &cameraTrauma, 0, 1);
		ImGui::SliderFloat("acceleration", &cameraAcceleration, 0, 0.2f);
		ImGui::SliderFloat("offset", &cameraOffsetFactor, 10, 50);
	}
	ImGui::End();
}
void Playing_Render()
{
	ClearBackground(BLACK);

	float shake = Clamp01(cameraTrauma);
	shake *= shake;

	Camera2D shakyCam = camera;
	float shakyTime = 100 * (float)GetTime();
	shakyCam.rotation += MAX_SHAKE_ROTATION * RAD2DEG * shake * PerlinNoise1(0, shakyTime);
	shakyCam.offset.x += MAX_SHAKE_TRANSLATION * shake * PerlinNoise1(1, shakyTime);
	shakyCam.offset.y += MAX_SHAKE_TRANSLATION * shake * PerlinNoise1(2, shakyTime);
	BeginMode2D(shakyCam);
	{
		// Draw objects back-to-front ordered by z ("Painter's algorithm").
		List(Object *) sorted = GetZSortedObjects();
		for (int i = ListCount(sorted) - 1; i >= 0; --i)
			Render(sorted[i]);
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
	int prevParagraphIndex = paragraphIndex;
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

	if (paragraphIndex != prevParagraphIndex)
		script->commandIndex = 0;

	UpdateCameraShake();
}
void Talking_Render()
{
	CallPreviousGameStateRender();

	Script *script = talkingObject->script;
	Paragraph paragraph = script->paragraphs[paragraphIndex];
	const char *speaker = paragraph.speaker;
	if (not paragraph.speaker)
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

bool showGrid = true;

void DrawGrid()
{
	Vector2 topLeftOnScreen = { 0, 0 };
	Vector2 topLeftInWorld = GetScreenToWorld2D(topLeftOnScreen, camera);
	Vector2 bottomRightOnScreen = { WINDOW_WIDTH, WINDOW_HEIGHT };
	Vector2 bottomRightInWorld = GetScreenToWorld2D(bottomRightOnScreen, camera);

	Vector2 cell0 = RoundDownToGridCell(topLeftInWorld);
	Vector2 cell1 = { cell0.x, bottomRightInWorld.y };
	cell1.y += GRID_RESOLUTION_Y - Wrap(cell1.y, 0, GRID_RESOLUTION_Y);

	float dy0 = bottomRightInWorld.y - cell0.y;
	float xIntercept0 = cell0.x - dy0 / Y_SQUISH;
	for (float x0 = xIntercept0; x0 <= bottomRightInWorld.x; x0 += GRID_RESOLUTION_X)
	{
		float x1 = x0 + dy0 / Y_SQUISH;
		Vector2 a0 = { x0, bottomRightInWorld.y };
		Vector2 a1 = { x1, cell0.y };
		DrawLineV(a0, a1, ColorAlpha(GRAY, 0.2f));
	}

	float dy1 = cell1.y - topLeftInWorld.y;
	float xIntercept1 = cell1.x - dy1 / Y_SQUISH;
	for (float x0 = xIntercept1; x0 <= bottomRightInWorld.x; x0 += GRID_RESOLUTION_X)
	{
		float x1 = x0 + dy1 / Y_SQUISH;
		Vector2 a0 = { x0, topLeftInWorld.y };
		Vector2 a1 = { x1, cell1.y };
		DrawLineV(a0, a1, ColorAlpha(GRAY, 0.2f));
	}
}
void DrawGridCell(Vector2 gridPoint, Color color)
{
	gridPoint.x = floorf(gridPoint.x);
	gridPoint.y = floorf(gridPoint.y);
	Vector2 g00 = { gridPoint.x + 0, gridPoint.y + 0 };
	Vector2 g01 = { gridPoint.x + 1, gridPoint.y + 0 };
	Vector2 g10 = { gridPoint.x + 0, gridPoint.y + 1 };
	Vector2 g11 = { gridPoint.x + 1, gridPoint.y + 1 };
	Vector2 s00 = GridToWorld(g00);
	Vector2 s01 = GridToWorld(g01);
	Vector2 s10 = GridToWorld(g10);
	Vector2 s11 = GridToWorld(g11);
	rlDrawRenderBatchActive();
	rlBegin(RL_QUADS);
	{
		rlColor4ub(color.r, color.g, color.b, color.a);
		rlVertex2f(s00.x, s00.y);
		rlVertex2f(s10.x, s10.y);
		rlVertex2f(s11.x, s11.y);
		rlVertex2f(s01.x, s01.y);
	}
	rlEnd();
	rlDrawRenderBatchActive();
}
void Editor_Update()
{
	if (input.console.wasPressed)
	{
		// resets the "FocusOnLoad" bool
		ResetConsole();
		PopGameState();
		return;
	}
}
void Editor_Render()
{
	ClearBackground(BLACK);

	BeginMode2D(camera);
	{
		static Object *pressedObject;
		static Object *selectedObject;
		static Object *draggedObject;
		static Vector2 draggedObjectFreeformPosition;

		bool isInObjectsTab = false;
		bool isInStairsTab = false;
		if (ImGui::Begin("Editor"))
		{
			ImGui::BeginTabBar("Tabs");
			{
				if (ImGui::BeginTabItem("Console"))
				{
					ShowConsoleGui();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Objects"))
				{
					isInObjectsTab = true;
					ImGui::BeginTable("Columns", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable);
					ImGui::TableSetupColumn(TempFormat("Objects %d/%d", numObjects, (int)COUNTOF(objects)));
					ImGui::TableSetupColumn("Properties");
					ImGui::TableHeadersRow();
					ImGui::TableNextRow();
					{
						ImGui::TableNextColumn();
						ImGui::Spacing();
						{
							ImGui::BeginTable("Controls", 3, ImGuiTableFlags_SizingStretchProp);
							{
								for (int i = 0; i < numObjects; ++i)
								{
									ImGui::TableNextRow();
									ImGui::PushID(i);
									{
										Object *object = &objects[i];
										bool selected = selectedObject == &objects[i];

										ImGui::TableNextColumn();
										if (i == 0)
											ImGui::BeginDisabled();
										ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(180, 20, 20, 255));
										ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(240, 20, 20, 255));
										ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(150, 20, 20, 255));
										if (ImGui::Button("x") or (i > 0 and selected and IsKeyPressed(KEY_DELETE)))
										{
											if (i == numObjects - 1)
											{
												if (numObjects <= 1)
													selectedObject = NULL;
												else
													selectedObject = &objects[i - 1];
											}

											Destroy(&objects[i]);
											CopyBytes(&objects[i], &objects[i + 1], (numObjects - i - 1) * sizeof objects[i]);
											--numObjects;
											selected = false;
											object = &objects[i];
										}
										ImGui::PopStyleColor(3);
										if (i == 0)
											ImGui::EndDisabled();

										ImGui::TableNextColumn();
										if (ImGui::Selectable(object->name, &selected))
											selectedObject = object;

										ImGui::TableNextColumn();
										if (ImGui::Button("Clone"))
										{
											if (numObjects < COUNTOF(objects))
											{
												char cloneName[sizeof object->name];
												CopyString(cloneName, object->name, sizeof cloneName);
												for (int suffix = 2; suffix < 100 and FindObjectByName(cloneName); ++suffix)
													FormatString(cloneName, sizeof cloneName, "%s%d", object->name, suffix);

												CopyBytes(&objects[i + 2], &objects[i + 1], (numObjects - i - 1) * sizeof objects[i]);
												++numObjects;
												Clone(&objects[i], &objects[i + 1]);
												CopyString(objects[i + 1].name, cloneName, sizeof objects[i + 1].name);
											}
										}
									}
									ImGui::PopID();
								}

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::TableNextColumn();
								if (ImGui::Button("+", ImVec2(ImGui::GetContentRegionAvail().x, 0)) and numObjects < COUNTOF(objects))
								{
									Object *object = &objects[numObjects++];
									memset(object, 0, sizeof object[0]);
									int index = numObjects;
									FormatString(object->name, sizeof object->name, "Object%d", index);
								}
							}
							ImGui::EndTable();
						}

						ImGui::TableNextColumn();
						ImGui::Spacing();
						{
							if (selectedObject)
							{
								ImGui::InputText("Name", selectedObject->name, sizeof selectedObject->name);
								ImGui::DragFloat2("Position", &selectedObject->position.x);

								const char *direction = GetDirectionString(selectedObject->direction);
								bool directionIsValid = GetCurrentSprite(selectedObject) != NULL;
								if (not directionIsValid)
								{
									ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 1, 0, 0, 1 });
									ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4{ 1, 0, 0, 1 });
									ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4{ 1, 0, 0, 1 });
								}
								ImGui::SliderInt("Direction", (int *)&selectedObject->direction, 0, DIRECTION_ENUM_COUNT - 1, direction);
								if (not directionIsValid)
									ImGui::PopStyleColor(3);

								ImGui::DragFloat("Z Offset", &selectedObject->zOffset);

								char scriptPath[256];
								CopyString(scriptPath, GetAssetPath(selectedObject->script), sizeof scriptPath);
								if (ImGui::InputText("Script", scriptPath, sizeof scriptPath, ImGuiInputTextFlags_EnterReturnsTrue))
								{
									ReleaseAsset(selectedObject->script);
									selectedObject->script = AcquireScript(scriptPath, roboto, robotoBold, robotoItalic, robotoBoldItalic);
								}

								char collisionMapPath[256];
								CopyString(collisionMapPath, GetAssetPath(selectedObject->collisionMap), sizeof collisionMapPath);
								if (ImGui::InputText("Collision map", collisionMapPath, sizeof collisionMapPath, ImGuiInputTextFlags_EnterReturnsTrue))
								{
									ReleaseAsset(selectedObject->collisionMap);
									selectedObject->collisionMap = AcquireCollisionMap(collisionMapPath);
								}

								if (ImGui::CollapsingHeader("Sprites"))
								{
									for (int dir = 0; dir < DIRECTION_ENUM_COUNT; ++dir)
									{
										char spritePath[256];
										CopyString(spritePath, GetAssetPath(selectedObject->sprites[dir]), sizeof spritePath);

										if (ImGui::InputText(TempFormat("%s", GetDirectionString((Direction)dir)), spritePath, sizeof spritePath, ImGuiInputTextFlags_EnterReturnsTrue))
										{
											ReleaseAsset(selectedObject->sprites[dir]);
											selectedObject->sprites[dir] = AcquireSprite(spritePath);
										}
									}
								}

								if (ImGui::CollapsingHeader("Expressions"))
								{
									for (int i = 0; i < COUNTOF(selectedObject->expressions); ++i)
									{
										ImGui::PushID(i);
										ImGui::BeginTable("ExpressionTable", 2, ImGuiTableFlags_SizingStretchProp);
										ImGui::TableNextRow();
										{
											Expression *expression = &selectedObject->expressions[i];

											ImGui::TableNextColumn();
											ImGui::InputText("Name", expression->name, sizeof expression->name);

											char portraitPath[256];
											CopyString(portraitPath, GetAssetPath(expression->portrait), sizeof portraitPath);
											ImGui::TableNextColumn();
											if (ImGui::InputText("Portrait", portraitPath, sizeof portraitPath, ImGuiInputTextFlags_EnterReturnsTrue))
											{
												ReleaseAsset(expression->portrait);
												expression->portrait = AcquireTexture(portraitPath);
											}
										}
										ImGui::EndTable();
										ImGui::PopID();
									}
								}
							}
						}
					}
					ImGui::EndTable();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Stairs"))
				{
					isInStairsTab = true;
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}
		ImGui::End();

		List(Object *) sorted = GetZSortedObjects();
		for (int i = ListCount(sorted) - 1; i >= 0; --i)
		{
			Object *object = sorted[i];
			Render(object);

			// Draw an outline around the object.
			Rectangle outline = GetOutline(object);

			Color outlineColor = GrayscaleAlpha(0.5f, 0.5f);
			float outlineThickness = 2;
			if (object == selectedObject)
			{
				float blend = (float)(0.5 * (1 + cos(10 * GetTime())));
				outlineThickness = 3;
				outlineColor = ColorAlpha(BlendColors(GREEN, DARKGREEN, blend), 0.5f);
			}
			outline = ExpandRectangle(outline, outlineThickness);
			DrawRectangleLinesEx(outline, outlineThickness, outlineColor);

			float z = GetFootPositionInScreenSpace(object).y + object->zOffset;
			Vector2 zLinePos0 = { outline.x, z };
			Vector2 zLinePos1 = { outline.x + outline.width, z };
			DrawLineEx(zLinePos0, zLinePos1, 2, YELLOW);
		}

		if (showGrid and draggedObject)
			DrawGrid();

		if (isInStairsTab)
		{
			DrawGrid();
			DrawGridCell(ScreenToGrid(GetMousePosition()), GRAY);
		}

		for (int i = 0; i < numStairs; ++i)
		{
			Stair stair = stairs[i];
			Vector2 gridPoint = { (float)stair.gridX, (float)stair.gridY };
			DrawGridCell(gridPoint, YELLOW);
		}

		if (not ImGui::GetIO().WantCaptureMouse)
		{
			Object *hoveredObject = FindObjectAtPosition(GetMousePositionInWorld());
			if (isInObjectsTab)
			{
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					pressedObject = hoveredObject;
					if (pressedObject == selectedObject)
					{
						draggedObject = hoveredObject;
						draggedObjectFreeformPosition = draggedObject->position;
					}
				}
				if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
				{
					if (hoveredObject == pressedObject)
						selectedObject = hoveredObject;
					draggedObject = NULL;
				}

				if (draggedObject)
				{
					Vector2 delta = GetMouseDelta();
					draggedObjectFreeformPosition.x += delta.x / camera.zoom;
					draggedObjectFreeformPosition.y += delta.y / camera.zoom;
					draggedObject->position = draggedObjectFreeformPosition;
					if (showGrid)
						draggedObject->position = SnapToGrid(draggedObjectFreeformPosition);
				}
			}
			else if (isInStairsTab)
			{
				int deltaElevation = 
					(int)IsMouseButtonPressed(MOUSE_BUTTON_LEFT) -
					(int)IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
				if (deltaElevation != 0)
				{
					Vector2 gridPoint = ScreenToGrid(GetMousePosition());
					Stair *stair = GetStairAt(gridPoint);
					if (not stair and numStairs < COUNTOF(stairs))
						stair = &stairs[numStairs++];
					if (stair)
					{
						stair->gridX = (int)floorf(gridPoint.x);
						stair->gridY = (int)floorf(gridPoint.y);
						stair->elevation += deltaElevation;
					}
				}
			}

			if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
			{
				Vector2 delta = GetMouseDelta();
				camera.target.x -= delta.x / camera.zoom;
				camera.target.y -= delta.y / camera.zoom;
			}

			if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) or (selectedObject and selectedObject == hoveredObject) or draggedObject)
				SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
			else
				SetMouseCursor(MOUSE_CURSOR_DEFAULT);

			float wheel = GetMouseWheelMove();
			if (wheel > 0)
				ZoomCameraToScreenPoint(GetMousePosition(), 1.1f);
			else if (wheel < 0)
				ZoomCameraToScreenPoint(GetMousePosition(), 1 / 1.1f);
		}
		else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

		if (not ImGui::GetIO().WantCaptureKeyboard)
		{
			if (IsKeyPressed(KEY_C))
				CenterCameraOn(player);
		}

		bool controlIsDown = IsKeyDown(KEY_LEFT_CONTROL) or IsKeyDown(KEY_RIGHT_CONTROL);
		if (IsKeyPressed(KEY_D) and controlIsDown)
			selectedObject = NULL;
		if (IsKeyPressed(KEY_S) and controlIsDown)
			SaveScene(lastSavedOrLoadedScene);
		if (IsKeyPressed(KEY_R) and controlIsDown)
			LoadScene(lastSavedOrLoadedScene);
		if (IsKeyPressed(KEY_G) and controlIsDown)
			showGrid = not showGrid;
	}
	EndMode2D();
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
	SetConfigFlags(FLAG_MSAA_4X_HINT);
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
		
		MapKeyToInputButton(KEY_F1, &input.console);
	}

	roboto = LoadFontAscii("roboto.ttf", 32);
	robotoBold = LoadFontAscii("roboto-bold.ttf", 32);
	robotoItalic = LoadFontAscii("roboto-italic.ttf", 32);
	robotoBoldItalic = LoadFontAscii("roboto-bold-italic.ttf", 32);

	LoadScene("test.scene");

	AddCommand("tp", HandlePlayerTeleportCommand, "tp x:float y:float  -  Teleport player");
	AddCommand("dev", HandleToggleDevModeCommand, "dev [value:bool]  -  Toggle developer mode.");
	AddCommand("shake", HandleCameraShakeCommand, "shake [trauma:float] [falloff:float]  -  Trigger camera shake.");
	AddCommand("sound", HandleSoundCommand,       "sound filename:string [volume:float] [pitch:float]  -  Play a sound.");
	AddCommand("moveto", HandleMoveTo, "moveto dx:float dy:float  -  Start moving the player to a position.");
	AddCommand("moveby", HandleMoveBy, "moveby dx:float dy:float  -  Start moving the player by a relative amount.");
	AddCommand("save", HandleSaveCommand, "save [filename:string]  -  Saves current scene to a file.");
	AddCommand("load", HandleLoadCommand, "load [filename:string]  -  Load a scene file.");

	SetCurrentGameState(GAMESTATE_PLAYING, NULL);
}
