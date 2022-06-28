#include "../core.h"

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