#pragma once

#include "lib/raylib.h"
#include "lib/raymath.h"
#include "lib/rlgl.h"
#include "lib/raygui.h"
#include <stdarg.h>
#include <limits.h>
#include <float.h>
#include <math.h>

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

//
// Runtime (these functions are called from runtime.c, but are defined in main.c).
//

void GameInit(void);
void GameLoopOneIteration(void);