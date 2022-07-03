#pragma once

#include "lib/raylib.h"
#include "lib/raymath.h"
#include "lib/rlgl.h"
#include "lib/raygui.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <iso646.h>
#include <math.h>

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

// Return number of items in a static array (DOESN'T WORK FOR POINTERS!).
#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))

// Utility macro to define an enum without having to type all this stuff out every time.
#define ENUM(name) typedef enum name name; enum name

// Utility macro to define a union without having to type all this stuff out every time.
#define UNION(name) typedef union name name; union name

// Utility macro to define a struct without having to type all this stuff out every time.
#define STRUCT(name) typedef struct name name; struct name

// Silence compiler warnings about unused variables or parameters.
#define UNUSED(x) ((void)(x))

// Converts megabytes to number of bytes.
#define MEGABYTES(x) (1024 * 1024 * (x))

// Converts kilobytes to number of bytes.
#define KILOBYTES(x) (1024 * (x))

// Use this on printf format string-like function parameters. The compiler will then issue errors if the arguments don't match the format string.
#define FORMAT_STRING _Printf_format_string_ const char *

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
// Hot-reload
//

STRUCT(FileData)
{
	void *bytes; // Note that this is NOT 0 terminated. So you can't use it as a string.
	int size;
};

// Load the entire file as bytes. The returned data will be automatically updated whenever the file changes.
FileData *LoadFileAndTrackChanges(const char *path);

// Loads the texture from a file. The returned texture will be automatically updated whenever the file changes.
Texture *LoadTextureAndTrackChanges(const char *path);

// Unloads a tracked file, and stops tracking it's changes.
void UnloadTrackedFile(FileData **data);

// Unloads a tracked texture, and stops tracking it's changes.
void UnloadTrackedTexture(Texture **texture);

// Checks all tracked items and hot-reloads any that changed. Automatically called at the start of each frame.
void HotReloadAllTrackedItems(void);

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
float ValueNoise2V(unsigned seed, Vector2 pos);
float ValueNoise3V(unsigned seed, Vector3 pos);

// Determinsically produces a uniform [-1, +1] float based on the inputs. The returned values, and their derivatives, are continuous.
float PerlinNoise1(unsigned seed, float x);
float PerlinNoise2(unsigned seed, float x, float y);
float PerlinNoise3(unsigned seed, float x, float y, float z);
float PerlinNoise2V(unsigned seed, Vector2 pos);
float PerlinNoise3V(unsigned seed, Vector3 pos);

//
// Math
//

// Returns a unit length vector pointing in the given angle (in radians).
Vector2 UnitVector2WithAngle(float angle);

// Returns a vector with the given polar coordinates.
Vector2 Vector2FromPolar(float length, float angle);

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

//
// Runtime (these are used in runtime.c, but they're actually defined in main.c and tests.c)
//

void Game(void);
void GameLoopOneIteration(void);

#ifdef __cplusplus
}
#endif
