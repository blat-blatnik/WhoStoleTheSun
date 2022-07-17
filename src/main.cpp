#include "core.h"
#include "lib/imgui/imgui.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_CENTER_X (0.5f*WINDOW_WIDTH)
#define WINDOW_CENTER_Y (0.5f*WINDOW_HEIGHT)
#define MAX_SHAKE_ROTATION (5*DEG2RAD)
#define MAX_SHAKE_TRANSLATION 50.0f
#define DEFAULT_CAMERA_SHAKE_TRAUMA 0.5f
#define DEFAULT_CAMERA_SHAKE_FALLOFF (0.7f * FRAME_TIME)
#define Y_SQUISH 0.8f

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
	int numExpressions;
	Expression expressions[10]; // We might want more, but this should generally be a very small number.
};

bool devMode = true; // @TODO @SHIP: Disable this for release.
Input input;
Font roboto;
Font robotoBold;
Font robotoItalic;
Font robotoBoldItalic;
Object objects[100];
Object *player; // Always the first object, i.e. player == &objects[0].
int numObjects;
Camera2D camera;
float cameraTrauma;
float cameraTraumaFalloff;

List(Texture *) LoadAllTexturesFromDirectory(const char *path)
{
	List(Texture *) textures = NULL;
	FilePathList files = LoadDirectoryFiles(path);
	{
		for (unsigned int i = 0; i < files.count; ++i)
			ListAdd(&textures, AcquireTexture(files.paths[i]));
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
	for (int i = 0; i < numObjects; ++i)
		if (StringsEqualNocase(objects[i].name, name))
			return &objects[i];

	return NULL;
}
Texture GetCharacterPortrait(const Object *object, const char *name)
{
	for (int i = 0; i < object->numExpressions; ++i)
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
	for (int i = 0; i < from->numExpressions; ++i)
		to->expressions[i].portrait = (Texture *)CloneAsset(from->expressions[i].portrait);
	for (int direction = 0; direction < DIRECTION_ENUM_COUNT; ++direction)
		to->sprites[direction] = (Sprite *)CloneAsset(from->sprites[direction]);
}
void Destroy(Object *object)
{
	ReleaseAsset(object->script);
	ReleaseAsset(object->collisionMap);
	for (int i = 0; i < object->numExpressions; ++i)
		ReleaseAsset(object->expressions[i].portrait);
	for (int direction = 0; direction < DIRECTION_ENUM_COUNT; ++direction)
		ReleaseAsset(object->sprites[direction]);
	ZeroBytes(object, sizeof object[0]);
}
void Update(Object *object)
{
	Sprite *sprite = GetCurrentSprite(object);
	if (not sprite)
		return;

	float animationFrameTime = 1 / object->animationFps;
	object->animationTimeAccumulator += FRAME_TIME;
	while (object->animationTimeAccumulator > animationFrameTime)
	{
		object->animationTimeAccumulator -= animationFrameTime;
		object->animationFrame = (object->animationFrame + 1) % sprite->numFrames;
	}
}
void Render(Object *object)
{
	Sprite *sprite = GetCurrentSprite(object);
	if (not sprite)
		return;

	if (sprite == object->sprites[object->direction])
		DrawTextureCentered(sprite->frames[object->animationFrame], object->position, WHITE);
	else
		DrawTextureCenteredAndFlippedVertically(sprite->frames[object->animationFrame], object->position, WHITE);
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

		// In the isometric perspective, the y direction is squished down a little bit.
		Vector2 feetPos = player->position;
		feetPos.y += 0.5f * GetCurrentTexture(player)->height;
		Vector2 newFeetPos = MovePointWithCollisions(feetPos, deltaPos);
		player->position = player->position + (newFeetPos - feetPos);;
	}

	for (int i = 0; i < numObjects; i++)
		Update(&objects[i]);

	CenterCameraOn(player);
	UpdateCameraShake();

	ImGui::Begin("Shake");
	{
		ImGui::SliderFloat("trauma", &cameraTrauma, 0, 1);
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

void Editor_Update()
{
	if (input.console.wasPressed)
	{
		PopGameState();
		return;
	}
}
void Editor_Render()
{
	CallPreviousGameStateRender();
	BeginMode2D(camera);
	{
		static Object *selectedObject = NULL;
		static Object *draggedObject = NULL;

		bool isInObjectsTab = false;
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
											}
										}

										// Draw an outline around the object.
										Rectangle outline = GetOutline(object);

										Color outlineColor = GrayscaleAlpha(0.5f, 0.5f);
										float outlineThickness = 2;
										if (selected)
										{
											outlineThickness = 3;
											outlineColor = ColorAlpha(GREEN, 0.5f);
										}
										outline = ExpandRectangle(outline, outlineThickness);
										DrawRectangleLinesEx(outline, outlineThickness, outlineColor);

										float z = GetFootPositionInScreenSpace(object).y + object->zOffset;
										Vector2 zLinePos0 = { outline.x, z };
										Vector2 zLinePos1 = { outline.x + outline.width, z };
										DrawLineEx(zLinePos0, zLinePos1, 2, YELLOW);
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
							}
						}
					}
					ImGui::EndTable();
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}
		ImGui::End();

		if (not ImGui::GetIO().WantCaptureMouse)
		{
			Object *hoveredObject = FindObjectAtPosition(GetMousePositionInWorld());
			if (isInObjectsTab)
			{
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					selectedObject = hoveredObject;
					draggedObject = hoveredObject;
				}
				if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
					draggedObject = NULL;

				if (draggedObject)
				{
					Vector2 delta = GetMouseDelta();
					draggedObject->position.x += delta.x / camera.zoom;
					draggedObject->position.y += delta.y / camera.zoom;
				}
			}

			if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
			{
				Vector2 delta = GetMouseDelta();
				camera.target.x -= delta.x / camera.zoom;
				camera.target.y -= delta.y / camera.zoom;
			}

			if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) or selectedObject == hoveredObject or draggedObject)
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
			if (IsKeyPressed(KEY_D) and (IsKeyDown(KEY_LEFT_CONTROL) or IsKeyDown(KEY_RIGHT_CONTROL)))
				selectedObject = NULL;
			if (IsKeyPressed(KEY_C))
				CenterCameraOn(player);
		}
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

#include <malloc.h>

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

	roboto = LoadFontAscii("roboto.ttf", 32);
	robotoBold = LoadFontAscii("roboto-bold.ttf", 32);
	robotoItalic = LoadFontAscii("roboto-italic.ttf", 32);
	robotoBoldItalic = LoadFontAscii("roboto-bold-italic.ttf", 32);
	
	player = &objects[numObjects++];
	CopyString(player->name, "Player", sizeof player->name);
	player->position.x = 1280 / 2;
	player->position.y = 720 / 2;
	player->direction = DIRECTION_DOWN;
	player->sprites[DIRECTION_RIGHT     ] = AcquireSprite("player_right");
	player->sprites[DIRECTION_UP_RIGHT  ] = AcquireSprite("player_up_right");
	player->sprites[DIRECTION_UP        ] = AcquireSprite("player_up");
	player->sprites[DIRECTION_UP_LEFT   ] = AcquireSprite("player_up_left");
	player->sprites[DIRECTION_LEFT      ] = AcquireSprite("player_left");
	player->sprites[DIRECTION_DOWN_LEFT ] = AcquireSprite("player_down_left");
	player->sprites[DIRECTION_DOWN      ] = AcquireSprite("player_down");
	player->sprites[DIRECTION_DOWN_RIGHT] = AcquireSprite("player_down_right");
	player->expressions[0].portrait = AcquireTexture("player-neutral.png");
	CopyString(player->expressions[0].name, "neutral", sizeof player->expressions[0].name);
	player->numExpressions = 1;
	
	Object *background = &objects[numObjects++];
	CopyString(background->name, "Background", sizeof background->name);
	background->sprites[0] = AcquireSprite("background.png");
	background->collisionMap = AcquireCollisionMap("collision-map.png");
	background->position.x = 0.5f * background->sprites[0]->frames[0].width;
	background->position.y = 0.5f * background->sprites[0]->frames[0].height;
	background->zOffset = -1080;

	Object *pinkGuy = &objects[numObjects++];
	CopyString(pinkGuy->name, "Pink guy", sizeof pinkGuy->name);
	pinkGuy->sprites[0] = AcquireSprite("pink-guy.png");
	pinkGuy->position.x = 700;
	pinkGuy->position.y = 250;

	Script pink = LoadScript("example-script.txt", roboto, robotoBold, robotoItalic, robotoBoldItalic);
	pinkGuy->script = AcquireScript("example-script.txt", roboto, robotoBold, robotoItalic, robotoBoldItalic);

	for (int i = 0; i < 10000; ++i)
	{
		//char *memory = (char *)MemRealloc(NULL, 64);
		//CopyBytes(memory, "Hello, sailor!!!", 16);
		//CopyBytes(memory + 16, "Hello, sailor!!!", 16);
		//CopyBytes(memory + 32, "Hello, sailor!!!", 16);
		//CopyBytes(memory + 48, "Hello, sailor!!!", 16);
	
		char *list1 = NULL;
		char *list2 = NULL;
		ListReserve(&list1, 64);
		ListReserve(&list2, 256);
	}

	pinkGuy->expressions[0].portrait = AcquireTexture("pink-guy-neutral.png");
	pinkGuy->expressions[1].portrait = AcquireTexture("pink-guy-happy.png");
	pinkGuy->expressions[2].portrait = AcquireTexture("pink-guy-sad.png");
	CopyString(pinkGuy->expressions[0].name, "neutral", sizeof pinkGuy->expressions[0].name);
	CopyString(pinkGuy->expressions[1].name, "happy", sizeof pinkGuy->expressions[1].name);
	CopyString(pinkGuy->expressions[2].name, "sad", sizeof pinkGuy->expressions[2].name);
	pinkGuy->numExpressions = 3;

	Object *greenGuy = &objects[numObjects++];
	CopyString(greenGuy->name, "Green guy", sizeof greenGuy->name);
	greenGuy->sprites[0] = AcquireSprite("green-guy.png");
	greenGuy->position.x = 1000;
	greenGuy->position.y = 250;
	greenGuy->script = AcquireScript("green-guy-script.txt", roboto, robotoBold, robotoItalic, robotoBoldItalic);
	greenGuy->expressions[0].portrait = AcquireTexture("green-guy-neutral.png");
	CopyString(greenGuy->expressions[0].name, "neutral", sizeof greenGuy->expressions[0].name);
	greenGuy->numExpressions = 1;

	Object *alex = &objects[numObjects++];
	CopyString(alex->name, "Alex", sizeof alex->name);
	alex->sprites[0] = AcquireSprite("alex");
	alex->position.x = 915;
	alex->position.y = 120;
	alex->animationFps = 15;
	alex->direction = DIRECTION_LEFT;
	alex->script = AcquireScript("alex-script.txt", roboto, robotoBold, robotoItalic, robotoBoldItalic);
	alex->expressions[0].portrait = AcquireTexture("alex-neutral.png");
	CopyString(alex->expressions[0].name, "neutral", sizeof alex->expressions[0].name);
	alex->numExpressions = 1;

	//Object *cauldron = &objects[numObjects++];
	//cauldron->name = "Cauldron";
	//cauldron->position.x = 1000;
	//cauldron->position.y = 550;
	//cauldron->spriteMgr.AddSprite("res/cauldron/");

	AddCommand("tp", HandlePlayerTeleportCommand, "tp x:float y:float - Teleport player->");
	AddCommand("dev", HandleToggleDevModeCommand, "dev [value:bool] - Toggle developer mode.");
	AddCommand("shake", HandleCameraShakeCommand, "shake [trauma:float] [falloff:float] - Trigger camera shake.");
	AddCommand("sound", HandleSoundCommand, "sound filename:string [volume:float] [pitch:float] - Play a sound.");

	SetCurrentGameState(GAMESTATE_PLAYING, NULL);
}
