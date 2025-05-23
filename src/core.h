#pragma once

#include "lib/raylib.h"
#include "lib/raymath.h"
#include "lib/rlgl.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <iso646.h>
#include <math.h>

#ifdef __cplusplus
#include "lib/imgui/imgui.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// _Printf_format_string_ is only defined on MSVC.
#ifndef _Printf_format_string_
#	define _Printf_format_string_
#endif

// __debugbreak() only works on MSVC.
#ifndef _MSC_VER
#	define __debugbreak()
#endif

#ifdef __cplusplus
#	define ENUM(name) enum name
#	define UNION(name) union name
#	define STRUCT(name) struct name
#else
#	define ENUM(name) typedef enum name name; enum name
#	define UNION(name) typedef union name name; union name
#	define STRUCT(name) typedef struct name name; struct name
#endif

// Return number of items in a static array (DOESN'T WORK FOR POINTERS!).
#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))

// Silence compiler warnings about unused variables or parameters.
#define UNUSED(x) ((void)(x))

// Converts megabytes to number of bytes.
#define MEGABYTES(x) (1024 * 1024 * (x))

// Converts kilobytes to number of bytes.
#define KILOBYTES(x) (1024 * (x))

// Use this on printf format string-like function parameters. The compiler will then issue errors if the arguments don't match the format string.
#define FORMAT_STRING _Printf_format_string_ const char *

// Concatenates two tokens without expanding macro arguments. E.g. PASTE_NOEXPAND(a, __LINE__) -> a__LINE__
#define PASTE_NOEXPAND(a, b) a##b

// Concatenates two tokens while expanding macro arguments. E.g. PASTE(a, __LINE__) -> a42
#define PASTE(a, b) PASTE_NOEXPAND(a, b)

//
// Constants
//

// (Fixed) frames per second the game runs at. You can assume that this never changes.
#define FPS 60

// (Fixed) amount of time that advanced between frames. The game must always hit this frame time, otherwise it will slow down.
#define FRAME_TIME (1.0f / FPS)

//
// List
//

// Use this to declare lists, e.g. List(int) myList = NULL; You can also do int *myList = NULL, but this makes it more distinct.
#define List(T) T*

// Sets the allocator used by the list. By default, lists use MemRealloc and MemFree (heap allocation).
void ListSetAllocator(List(void) *listPointer, void *(*realloc)(void *block, int newSize), void(*free)(void *block));

// Returns the number of items in the list.
int ListCount(const List(void) list);

// Returns the capacity of the list, which is the number of items the list can hold before having to resize.
int ListCapacity(const List(void) list);

// Deallocates all memory held by the list.
void ListDestroy(List(void) *listPointer);

// Ensures that the list has space for at least the given number of elements.
#define ListReserve(listPointer, neededCapacity)\
	private_ListReserve((List(void)*)(listPointer), (neededCapacity), sizeof (*listPointer)[0])

// Adds an item to the list.
#define ListAdd(listPointer, item) do{\
	int private_index = ListCount((const List(void))*(listPointer));\
	ListReserve((listPointer), private_index + 1);\
	(*(listPointer))[private_index] = (item);\
	++((int *)(*(listPointer)))[-1];\
}while(0)

// Allocates space for count items in the list, and returns a pointer to the newly allocated items.
// This can be more efficient and convenient than ListAdd for larger structures.
#define ListAllocate(listPointer, count)(\
	ListReserve((listPointer), ListCount(*listPointer) + (count)), \
	((int *)(*(listPointer)))[-1] += (count), \
	(*listPointer) + ListCount(*listPointer) - (count))

// Adds space for one more item in the list, and returns a pointer to the newly allocated item.
// This can be more efficient and convenient than ListAdd for larger structures.
#define ListAllocateItem(listPointer)\
	ListAllocate(listPointer, 1)

// Removes the last item in the list and returns it.
#define ListPop(listPointer)\
	(private_ListPop(listPointer), (*listPointer)[ListCount(*listPointer)])

// Removes the item at the given index in the list by swapping it with the last item in the list.
// This will destroy the order of the list, but if you don't care about the order, it's very fast.
#define ListSwapRemove(listPointer, index) do{\
	if ((index) < ListCount(*(listPointer)))\
		(*listPointer)[index] = (*listPointer)[--((int *)(*listPointer))[-1]];\
}while(0);\

// Implementation details..
void private_ListReserve(List(void) *listPointer, int neededCapacity, int sizeOfOneItem);
void private_ListPop(List(void) *listPointer);

//
// Logging
//

// Logs an informational message. You can use this like printf.
void LogInfo(FORMAT_STRING message, ...);

// Logs a warning message. For example when you detect something that could lead to problems in the future.
void LogWarning(FORMAT_STRING message, ...);

// Logs an error message. For example when you detect an unrecoverable error.
void LogError(FORMAT_STRING message, ...);

// Immediately terminates the program and displays an error message.
void Crash(FORMAT_STRING message, ...);

// Use this to make sure that a condition holds. If it doesn't you'll be brought into the debugger, as if by a breakpoint.
#define ASSERT(condition)do{\
	if (!(condition)) {\
		__debugbreak();\
		Crash("Assertion failed! '%s' in %s(), file %s, line %d.", #condition, __func__, __FILE__, __LINE__);\
	}\
}while(0)

//
// Memory utilities
//

// Clears all of the bytes.
void ZeroBytes(void *bytes, int count);

// Sets all of the bytes to a given value.
void SetBytes(void *bytes, unsigned char value, int count);

// Sets all of the ints to a given value.
void SetInts(int *ints, int value, int count);

// Sets all of the floats to a given value.
void SetFloats(float *floats, float value, int count);

// Copies the given number of bytes from one memory location to another.
void CopyBytes(void *to, const void *from, int numBytes);

// Swaps the bytes of a and b.
void SwapBytes(void *a, void *b, int numBytes);

// Returns true if all bytes from a and b are equal.
bool BytesEqual(const void *a, const void *b, int numBytes);

// Quick sorts items in place (non-stable).
void Sort(void *items, int numItems, int sizeofOneItem, int(*compare)(const void *left, const void *right));

//
// Char utilities
//

// Returns true if the character is whitespace: ' ', '\t', '\n', '\r', '\v'.
bool CharIsWhitespace(char c);

// If the char is uppercase, returns it's lowercase counterpart, otherwise returns the char unchanged.
char CharToLowercase(char c);

// If the char is lowercase, returns it's uppercase counterpart, otherwise returns the char unchanged.
char CharToUppercase(char c);

//
// String utilities
//

// Returns the length of the string in characters.
int StringLength(const char *string);

// Copies bytes from one string to another. The destination string is guaranteed to be 0 terminated at the end.
void CopyString(char *to, const char *from, int maxBytesToCopy);

// Prints a formatted string into the given buffer. The output is cut off if it is too long to fit in the buffer.
// The buffer is always guaranteed to be 0 terminated at the end.
void FormatString(char *buffer, int capacity, FORMAT_STRING format, ...);

// Same thing as FormatString but with an explicit varargs pack.
void FormatStringVa(char *buffer, int capacity, FORMAT_STRING format, va_list args);

// Checks whether the strings are equal.
bool StringsEqual(const char *a, const char *b);

// Checks whether the strings are equal - ignores the case of the characters, so 'A' and 'a' are considered equal.
bool StringsEqualNocase(const char *a, const char *b);

// Replace every occurence of the target character in the string.
void ReplaceChar(char *string, char target, char replacement);

// Skips all leading whitespace in a string.
char *SkipLeadingWhitespace(const char *string);

// Hashes a string using 32-bit FNV-1a.
unsigned HashString(const char *string);

// SplitByWhitespace("Hello sailor\n\t1 2  3") -> ["Hello", "sailor", "1", "2", "3"]. The result is allocated from temporary storage.
List(char*) SplitByWhitespace(const char* string);
List(char *) SplitByChar(const char *string, const char* spacer);

//
// Temporary allocator
//

// Allocates the given number of bytes from temporary storage. The returned pointer's lifetime is only valid in the current frame. 
// At the end of the frame, the pointer is automatically freed. DO NOT KEEP A TEMPORARY STORAGE POINTER BETWEEN FRAMES!
// You can call TempRealloc, TempFree, or TempReset to free the pointer earlier, if you want to conserve space.
void *TempAlloc(int numBytes);

// Reallocates a temporary memory block with a new size. This works exactly like realloc, but follows the lifetime rules of temporary storage.
void *TempRealloc(void *block, int numBytes);

// Frees a temporary memory block. This works exactly like free.
void TempFree(void *block);

// Returns the current temporary storage position.
// You can later call TempReset with the returned mark to free all temporary memory allocations made between the TempMark and the TempReset.
int TempMark(void);

// Frees all temporary storage allocations made after the given mark.
// Call TempReset(0) to free all allocated temporary memory. This is done once at the start of each frame.
void TempReset(int mark);

// Copies the given bytes to temporary storage.
void *TempCopy(const void *bytes, int numBytes);

// Creates a copy of the given string in temporary storage.
char *TempString(const char *string);

// Prints a formatted string into temporary storage.
char *TempFormat(FORMAT_STRING format, ...);

// Same thing as TempFormat but with an explicit varargs pack.
char *TempFormatVa(FORMAT_STRING format, va_list args);

//
// String Builder
//

STRUCT(StringBuilder)
{
	char *buffer;
	int cursor;
	int capacity;
};

// Initializes a string builder from a character array. Calling this is necessary in order to ensure the buffer is 0 terminated.
StringBuilder CreateStringBuilder(char buffer[], int capacity);

// Appends a character to the string builder, if it fits in the buffer.
void AppendChar(StringBuilder *builder, char c);

// Repeatedly appends the same character to the string builder. As many characters are appended as will fit.
void AppendCharRepeated(StringBuilder *builder, char c, int repeatCount);

// Appends a string to the string builder. If the whole string doesn't fit into the buffer, it will be cut off.
void AppendString(StringBuilder *builder, const char *string);

// Appends a printf formatted string to the string builder. If the string doesn't fit into the buffer, nothing will be appended.
void AppendFormat(StringBuilder *builder, FORMAT_STRING format, ...);

// Appends a printf formatted string to the string builder. If the string doesn't fit into the buffer, nothing will be appended.
void AppendFormatVa(StringBuilder *builder, FORMAT_STRING format, va_list args);

// Creates a string builder from a stack buffer with the given capacity. THIS ONLY WORKS IN C, NOT IN C++.
#define STRING_BUILDER_ON_STACK(capacity) CreateStringBuilder((char[capacity]){0}, (capacity))

//
// Binary Stream
//

STRUCT(BinaryStream)
{
	void *buffer;
	int size;   // Size of the buffer.
	int cursor; // Read/write cursor.
};

// Reads a 32-bit integer from the stream and advances the cursor by 4 bytes.
int ReadInt(BinaryStream *stream);

// Reads a 32-bit float from the stream and advances the cursor by 4 bytes.
float ReadFloat(BinaryStream *stream);

// Reads an 1 byte bool from the stream and advances the cursor by 1 byte.
bool ReadBool(BinaryStream *stream);

// Reads a 0-terminated string from the stream and returns a pointer to it, then advances the cursor by however many bytes were read.
const char *ReadString(BinaryStream *stream);

// Reads a fixed number of bytes from the stream and returns a pointer to them, then advances the cursor by that many bytes.
const void *ReadBytes(BinaryStream *stream, int numBytesToRead);

// Copies a fixed number of bytes from the stream into a buffer, then advances the cursor by the number of bytes read.
void ReadBytesInto(BinaryStream *stream, void *buffer, int numBytesToRead);

// Writes a 32-bit integer to the stream and advances the cursor by 4 bytes.
void WriteInt(BinaryStream *stream, int i);

// Writes a 8-bit bool to the stream and advances the cursor by 1 byte.
void WriteBool(BinaryStream *stream, bool b);

// Writes a 32-bit float to the stream and advances the cursor by 4 bytes.
void WriteFloat(BinaryStream *stream, float f);

// Writes a 0-terminated string to the stream and advances the cursor by however many bytes were written.
void WriteString(BinaryStream *stream, const char *s);

// Writes a fixed number of bytes to the stream and advances the cursor by however many bytes were written.
void WriteBytes(BinaryStream *stream, const void *bytes, int numBytesToWrite);

//
// Script
//

STRUCT(Paragraph)
{
	char *speaker;
	char *text; // [textLength] NOT 0 TERMINATED!
	int textLength;
	float duration;
	List(int) codepoints;
};

STRUCT(Script)
{
	Font font;
	Font boldFont;
	Font italicFont;
	Font boldItalicFont;
	int commandIndex; // Keeps track of which commands have already run so they don't run twice.
	char *text;
	List(char) stringPool; // This is where all expressions and speaker names are stored.
	List(Paragraph) paragraphs;
};

// Loads a script from the given text file.
Script LoadScript(const char *path, Font regular, Font bold, Font italic, Font boldItalic);

// Unloads all script memory and nullifies the script.
void UnloadScript(Script *script);

// Draws the script paragraph at the given index. NOTE: This also executes console commands within the paragraph!
void DrawScriptParagraph(Script *script, int paragraphIndex, Rectangle textBox, float fontSize, Color color, Color shadowColor, float time);

// Gets the current speaker expression at the given time in the paragraph.
const char *GetScriptExpression(Script script, int paragratphIndex, float time);

//
// Sprite
//

STRUCT(Sprite)
{
	int numFrames;
	Texture *frames;
};

Sprite LoadSprite(const char *path);

void UnloadSprite(Sprite sprite);

//
// Sounds
//

// Plays a sound immediately, the lifetime of the sound resource is managed automatically.
void PlayTemporarySound(const char *path);

// Plays a temporary sound with a given volume and pitch. 1 is the default for both.
void PlayTemporarySoundEx(const char *path, float volume, float pitch);

// Releases all temporary sounds that finished playing. Called once at the end of every frame.
void UpdateTemporarySounds(void);

//
// Asset manager
//

// Loads a collision map asset - currently a grayscale image.
Image *AcquireCollisionMap(const char *path);

// Loads a texture asset - we might remove this later and just use sprites.
Texture *AcquireTexture(const char *path);

Sprite *AcquireSprite(const char *path);

// Loads a script asset.
Script *AcquireScript(const char *path, Font regular, Font bold, Font italic, Font boldItalic);

Music *AcquireMusic(const char *path);

Sound *AcquireSound(const char *path);

// Releases any asset.
void ReleaseAsset(void *asset);

// Completely clones an asset, as if you has acquired it from scratch.
void *CloneAsset(void *asset);

// Returns the file/directory path of an asset.
const char *GetAssetPath(const void *asset);

// Hot-reloads any changed assets. This called once at the end of every frame.
void UpdateAllChangedAssets(void);

//
// Random
//

STRUCT(Random)
{
	unsigned seed;
	int index;
};

// Initializes a Random seeded with the current time.
Random TimeSeededRandom(void);

// Returns 32 random bits.
unsigned RandomBits(Random *rand);

// Returns a random integer in the range [min, max).
int RandomInt(Random *rand, int min, int max);

// Returns a random, uniformly distributed float in the range [min, max).
float RandomFloat(Random *rand, float min, float max);

// Returns a random, uniformly distributed float in the range [0, 1).
float RandomFloat01(Random *rand);

// Returns a random float from the normal distribution with the given mean and standard deviation.
float RandomNormal(Random *rand, float mean, float sdev);

//  Returns a random float from the standard normal distribution (mean: 0, sdev: 1).
float RandomNormal01(Random *rand);

// Returns true or false with 50/50 odds.
bool RandomBool(Random *rand);

// Has a given probability of returning true.
bool RandomProbability(Random *rand, float probabilityOfTrue);

// Returns a random integer from [0, numWeights). The probability of selecting index i is given by: weights[i] / sum(weights).
int RandomSelect(Random *rand, const float probabilityWeights[], int numWeights);

// Randomly shuffles the given array of items.
void RandomShuffle(Random *rand, void *items, int numItems, int sizeOfOneItem);

//
// Noise
//

// Deterministically produces 32 random bits based on the inputs.
unsigned BitNoise1(unsigned seed, int x);
unsigned BitNoise2(unsigned seed, int x, int y);
unsigned BitNoise3(unsigned seed, int x, int y, int z);

// Deterministically produces an integer in the range [min, max) based on the inputs.
int IntNoise1(unsigned seed, int min, int max, int x);
int IntNoise2(unsigned seed, int min, int max, int x, int y);
int IntNoise3(unsigned seed, int min, int max, int x, int y, int z);

// Deterministically produces a uniform float in the range [0, 1) based on the inputs.
float FloatNoise1(unsigned seed, int x);
float FloatNoise2(unsigned seed, int x, int y);
float FloatNoise3(unsigned seed, int x, int y, int z);

// Determinsically produces a uniform [-1, +1] float based on the inputs. The returned values are continuous, but the derivatives are not.
float ValueNoise1(unsigned seed, float x);
float ValueNoise2(unsigned seed, float x, float y);
float ValueNoise3(unsigned seed, float x, float y, float z);
float ValueNoise2V(unsigned seed, Vector2 position);
float ValueNoise3V(unsigned seed, Vector3 position);

// Determinsically produces a uniform [-1, +1] float based on the inputs. The returned values, and their derivatives, are continuous.
float PerlinNoise1(unsigned seed, float x);
float PerlinNoise2(unsigned seed, float x, float y);
float PerlinNoise3(unsigned seed, float x, float y, float z);
float PerlinNoise2V(unsigned seed, Vector2 position);
float PerlinNoise3V(unsigned seed, Vector3 position);

//
// Math
//

ENUM(Direction)
{
	DIRECTION_RIGHT,
	DIRECTION_UP_RIGHT,
	DIRECTION_UP,
	DIRECTION_UP_LEFT,
	DIRECTION_LEFT,
	DIRECTION_DOWN_LEFT,
	DIRECTION_DOWN,
	DIRECTION_DOWN_RIGHT,
	DIRECTION_ENUM_COUNT
};

// Returns the string name of a direction.
const char *GetDirectionString(Direction direction);

// Returns the direction but mirrored vertically. E.g. LEFT -> RIGHT, UP_LEFT -> UP_RIGHT.
Direction MirrorDirectionVertically(Direction direction);

// Returns the 8-sided direction that the vector matches most closely.
Direction DirectionFromVector(Vector2 v);

// Returns a unit length vector pointing in the given angle (in radians).
Vector2 UnitVector2WithAngle(float angle);

// Returns a vector with the given polar coordinates.
Vector2 Vector2FromPolar(float length, float angle);

// Returns the angle between the vector and the x-axis. I.e. atan2f(y, x).
float Vector2Atan(Vector2 v);

// Smoothly interpolates between 0 and 1 based on t. https://en.wikipedia.org/wiki/Smoothstep.
float Smoothstep(float edge0, float edge1, float t);

// Smoothly interpolates between 0 and 1 based on t. https://en.wikipedia.org/wiki/Smoothstep#Variations.
float Smootherstep(float edge0, float edge1, float t);

// Equivalent to Smoothstep(0, 1, t).
float Smoothstep01(float t);

// Equivalent to Smootherstep(0, 1, t).
float Smootherstep01(float t);

// Wraps a number in [0, 1).
float Wrap01(float x);

// Clamps a number to [0, 1].
float Clamp01(float x);

// Clamps an integer to [min, max].
int ClampInt(int x, int min, int max);

// Returns +1 if x is positive, -1 if x is negative, and 0 if x is 0.
float Sign(float x);

// Returns the center point of the rectangle.
Vector2 RectangleCenter(Rectangle rect);

// Expands a rectangle equally in all 4 directions.
Rectangle ExpandRectangle(Rectangle rect, float amount);

// Expands a rectangle horizontally and vertically.
Rectangle ExpandRectangleVh(Rectangle rect, float vertical, float horizontal);

// Expands a rectangle differently in all 4 directions.
Rectangle ExpandRectangleEx(Rectangle rect, float top, float bottom, float left, float right);

//
// Color
//

// Returns a brighter version of the color. E.g. Brighten(color, 2) will return a roughly 2x brighter color.
Color Brighten(Color color, float amount);

// Returns a darker version of the color. E.g. Darken(color, 2) will return a roughly 2x darker color.
Color Darken(Color color, float amount);

// Brightens the color by a default amount.
Color Brighter(Color color);

// Darkens the color by a default amount.
Color Darker(Color color);

// Returns the color with the oposite hue, and same saturation, value, and alpha as the input.
Color GetColorOfOpositeHue(Color color);

// Returns a grayscale color with the given [0=black, 1=white] intensity, and an alpha of 1.
Color Grayscale(float intensity);

// Returns a grayscale color with the given intensity and alpha value.
Color GrayscaleAlpha(float intensity, float alpha);

// Returns a color with the given RGB components in [0, 1].
Color FloatRGB(float red, float green, float blue);

// Returns a color with the given RGBA components in [0, 1].
Color FloatRGBA(float red, float green, float blue, float alpha);

// Linearly interpolates between c0 and c1.
Color BlendColors(Color c0, Color c1, float t);

// Samples a heatmap color palette with x in [-1, +1]. Negative x gives colder colors, positive x gives warmer colors.
Color HeatmapPalette(float x);

//
// Drawing
//

// Sets the current rlgl color.
void rlColor(Color color);

// Emits an rlgl vertex position.
void rlVertex2v(Vector2 p);

// Draws a single colored quad.
void DrawQuad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Color color);

// Draws a quad with a gradient.
void DrawQuadGradient(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Color color0, Color color1, Color color2, Color color3);

// Draws a texture centered at the given point.
void DrawTextureCentered(Texture texture, Vector2 position, Color tint);

// Draws a texture centered at the given point and flipped vertically.
void DrawTextureCenteredAndFlippedVertically(Texture texture, Vector2 position, Color tint);

void DrawTextureCenteredScaled(Texture texture, Vector2 position, float scale, Color tint);

//
// Text
//

// Loads all ASCII glyphs from the given .ttf file.
Font LoadFontAscii(const char *path, int fontSize);

// Returns the line height of a font for a particular font size.
float GetLineHeight(Font font, float fontSize);

// Draws a formatted string starting at (x, y) and going right and down.
void DrawFormat(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, ...);

// Same as DrawFormat but takes an explicit varargs pack.
void DrawFormatVa(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, va_list args);

// Draws a formatted string centered at (x, y).
void DrawFormatCentered(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, ...);

// Same as DrawFormatCentered but takes an explicit varargs pack.
void DrawFormatCenteredVa(Font font, float x, float y, float fontSize, Color color, FORMAT_STRING format, va_list args);

//
// Slab allocator
//

STRUCT(Slab)
{
	Slab *prev;
	Slab *next;
	void *memory;
	int cursor;
	int capacity;
};

STRUCT(SlabAllocator)
{
	char magic[4];
	Slab *slab;
	int cursor;
};

// Allocates memory from the allocator. The returned pointer will be aligned to a 16-byte boundary.
// The returned memory will be zeroed. If the requested byte count is 0 or negative, a 0 sized valid pointer is returned.
void *AllocateFromSlabAllocator(SlabAllocator *allocator, int numBytes);

// Reallocates a previously allocated memory block with a new size. If the memory block grows, the new bytes will be zeroed.
// If `block` is NULL, the call is equivalent to AllocateFromSlabAllocator(allocator, numBytes).
void *ReallocateFromSlabAllocator(SlabAllocator *allocator, void *block, int numBytes);

// Deallocates a previously allocated memory block. If `block` is NULL this function does nothing.
void FreeFromSlabAllocator(SlabAllocator *allocator, void *block);

// Frees all memory allocated from the allocator after the given cursor.
void ResetSlabAllocator(SlabAllocator *allocator, int cursor);

//
// Game states
//

// Associates callback functions to a game state ID.
// - init is called once, when the associated game state becomes current.
// - deinit is called once, when the associated game state is no longer current, or on the stack.
// - update and render are called once per frame while the associated game state is current.
void RegisterGameState(int state, void(*init)(void *parameter), void(*deinit)(void), void(*update)(void), void(*render)(void));

// Pushes the current game state onto the game state stack, and then initializes a new current game state with the given parameter.
void PushGameState(int state, void *parameter);

// Calls deinit on the current game state, and then replaces it with the game state popped off of the game state stack.
void PopGameState(void);

// Pops game states off of the stack until a particular game state becomes current.
void PopGameStateUntil(int state);

// Replaces the current game state without affecting the stack. Equivalent to a pop, followed by a push.
void SetCurrentGameState(int state, void *parameter);

// Calls the render function of the game state on top of the game state stack. The call is performed as if that game state was current.
void CallPreviousGameStateRender(void);

// Calls the update function of the current game state.
void UpdateCurrentGameState(void);

// Calls the render function of the current game state.
void RenderCurrentGameState(void);

// Returns the integer ID of the current game state.
int GetCurrentGameState(void);

// Returns the integer ID of the game state on top of the game state stack.
int GetPreviousGameState(void);

// Returns the number of update frames that elapsed in the current game state since init was called.
int GetFrameNumberInCurrentGameState(void);

// Returns the number of seconds that elapsed in the current game state since init was called.
double GetTimeInCurrentGameState(void);

// Sets the frame number of the current game state to a new value.
void SetFrameNumberInCurrentGameState(int frameNumber);

// Really stupid looking conveniance macro to allow defining a game state in once place.
#define REGISTER_GAME_STATE(name, init, deinit, update, render)\
	static int PASTE(dummy__, __LINE__) = [](){ RegisterGameState(name, init, deinit, update, render); return 0; }();

//
// Input
//

STRUCT(InputButton)
{
	bool isDown;
	bool wasPressed;
	bool wasReleased;
};

STRUCT(InputAxis)
{
	Vector2 position;
};

ENUM(InputMode)
{
	INPUT_MODE_KEYBOARD_AND_MOUSE,
	INPUT_MODE_CONTROLLER,
};

void MapKeyToInputButton(KeyboardKey key, InputButton *button);

void MapKeyToInputAxis(KeyboardKey key, InputAxis *axis, float xWhenPressed, float yWhenPressed);

void MapMouseButtonToInputButton(MouseButton mouseButton, InputButton *button);

void MapMouseButtonToInputAxis(MouseButton mouseButton, InputAxis *axis, float xWhenPressed, float yWhenPressed);

void MapGamepadButtonToInputButton(GamepadButton gamepadButton, InputButton *button);

void MapGamepadButtonToInputAxis(GamepadButton gamepadButton, InputAxis *axis, float xWhenPressed, float yWhenPressed);

void MapGamepadAxisToInputButton(GamepadAxis gamepadAxis, InputButton *button, float dotX, float dotY, float threshold);

void MapGamepadAxisToInputAxis(GamepadAxis gamepadAxis, InputAxis *axis);

void UpdateInputMappings(void);

//
// Console
//

typedef bool(*CommandHandler)(List(const char *) args);

bool ParseCommandBoolArg(const char *string, bool *outSuccess);

int ParseCommandIntArg(const char *string, bool *outSuccess);

float ParseCommandFloatArg(const char *string, bool *outSuccess);

void AddCommand(const char *command, CommandHandler handle, const char *help);

void ExecuteCommand(const char *command);

void ShowConsoleGui(void);

void ResetConsole(void);

//
// Runtime
//

// Initialize the game. This is used in runtime.cpp, but should actually be defined by the game.
void GameInit(void);

// Deinitialize the game. This is used in runtime.cpp, but should actually be defined by the game. CALLED ON DESKTOP PLATFORMS ONLY.
void GameDeinit(void);

#ifdef __cplusplus
}
inline Vector2 operator +(Vector2 v) { return v; }
inline Vector2 operator -(Vector2 v) { return { -v.x, -v.y }; }
inline Vector2 operator +(Vector2 left, Vector2 right) { return { left.x + right.x, left.y + right.y }; }
inline Vector2 operator -(Vector2 left, Vector2 right) { return { left.x - right.x, left.y - right.y }; }
inline Vector2 operator *(Vector2 left, Vector2 right) { return { left.x * right.x, left.y * right.y }; }
inline Vector2 operator /(Vector2 left, Vector2 right) { return { left.x / right.x, left.y / right.y }; }
inline Vector2 operator %(Vector2 left, Vector2 right) { return { fmodf(left.x, right.x), fmodf(left.y, right.y) }; }
inline Vector2 operator +(Vector2 left, float right) { return { left.x + right, left.y + right}; }
inline Vector2 operator -(Vector2 left, float right) { return { left.x - right, left.y - right}; }
inline Vector2 operator *(Vector2 left, float right) { return { left.x * right, left.y * right}; }
inline Vector2 operator /(Vector2 left, float right) { return { left.x / right, left.y / right }; }
inline Vector2 operator %(Vector2 left, float right) { return { fmodf(left.x, right), fmodf(left.y, right) }; }
inline Vector2 operator +(float left, Vector2 right) { return { left + right.x, left + right.y }; }
inline Vector2 operator -(float left, Vector2 right) { return { left - right.x, left - right.y }; }
inline Vector2 operator *(float left, Vector2 right) { return { left * right.x, left * right.y }; }
inline Vector2 operator /(float left, Vector2 right) { return { left / right.x, left / right.y }; }
inline Vector2 operator %(float left, Vector2 right) { return { fmodf(left, right.x), fmodf(left, right.y) }; }
inline bool operator ==(Vector2 v1, Vector2 v2) { if (v1.x == v2.x && v1.y == v2.y) return true; return false; }
#endif
