#include "../core.h"

const char *GetDirectionString(Direction direction)
{
	static const char *strings[DIRECTION_ENUM_COUNT] =
	{
		[DIRECTION_RIGHT     ] = "right",
		[DIRECTION_UP_RIGHT  ] = "up right",
		[DIRECTION_UP        ] = "up",
		[DIRECTION_UP_LEFT   ] = "up left",
		[DIRECTION_LEFT      ] = "left",
		[DIRECTION_DOWN_LEFT ] = "down left",
		[DIRECTION_DOWN      ] = "down",
		[DIRECTION_DOWN_RIGHT] = "down right",
	};
	return strings[direction];
}

Direction MirrorDirectionVertically(Direction direction)
{
	static Direction map[DIRECTION_ENUM_COUNT] =
	{
		[DIRECTION_RIGHT     ] = DIRECTION_LEFT,
		[DIRECTION_UP_RIGHT  ] = DIRECTION_UP_LEFT,
		[DIRECTION_UP        ] = DIRECTION_UP,
		[DIRECTION_UP_LEFT   ] = DIRECTION_UP_RIGHT,
		[DIRECTION_LEFT      ] = DIRECTION_RIGHT,
		[DIRECTION_DOWN_LEFT ] = DIRECTION_DOWN_RIGHT,
		[DIRECTION_DOWN      ] = DIRECTION_DOWN,
		[DIRECTION_DOWN_RIGHT] = DIRECTION_DOWN_LEFT,
	};
	return map[direction];
}

Direction DirectionFromVector(Vector2 v)
{
	float angle01 = Wrap(Vector2Atan(v), 0, 2 * PI) / (2 * PI);
	float floatIndex = roundf(angle01 * DIRECTION_ENUM_COUNT);
	int index = (int)floatIndex;
	if (index >= DIRECTION_ENUM_COUNT)
		index = 0;
	if (index < 0)
		index = DIRECTION_ENUM_COUNT - 1;
	return (Direction)index;
}

Vector2 UnitVector2WithAngle(float angle)
{
	return Vector2FromPolar(1, angle);
}

Vector2 Vector2FromPolar(float length, float angle)
{
	float s = sinf(angle);
	float c = cosf(angle);
	return (Vector2) { c * length, s * length };
}

float Vector2Atan(Vector2 v)
{
	return atan2f(v.y, v.x);
}

float Smoothstep(float edge0, float edge1, float t)
{
	return Smoothstep01((t - edge0) / (edge1 - edge0));
}

float Smootherstep(float edge0, float edge1, float t)
{
	return Smootherstep01((t - edge0) / (edge1 - edge0));
}

float Smoothstep01(float t)
{
	return t * t * (3 - 2 * t);
}

float Smootherstep01(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

float Wrap01(float x)
{
	return x - floorf(x);
}

float Clamp01(float x)
{
	if (x < 0)
		x = 0;
	if (x > 1)
		x = 1;
	return x;
}

int ClampInt(int x, int min, int max)
{
	if (x < min)
		x = min;
	if (x > max)
		x = max;
	return x;
}

float Sign(float x)
{
	return (float)((x > 0) - (x < 0));
}

Vector2 RectangleCenter(Rectangle rect)
{
	return (Vector2)
	{
		rect.x + 0.5f * rect.width,
		rect.y + 0.5f * rect.height,
	};
}

Rectangle ExpandRectangle(Rectangle rect, float amount)
{
	return ExpandRectangleEx(rect, amount, amount, amount, amount);
}

Rectangle ExpandRectangleVh(Rectangle rect, float vertical, float horizontal)
{
	return ExpandRectangleEx(rect, vertical, vertical, horizontal, horizontal);
}

Rectangle ExpandRectangleEx(Rectangle rect, float top, float bottom, float left, float right)
{
	return (Rectangle)
	{
		.x      = rect.x - left,
		.y      = rect.y - top,
		.width  = rect.width + left + right,
		.height = rect.height + top + bottom
	};
}