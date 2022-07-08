#include "../core.h"

static unsigned RandomBitsToRange(unsigned rand1, unsigned rand2, unsigned range)
{
	// https://github.com/apple/swift/pull/39143
	uint64_t r = (uint64_t)range;
	uint64_t a = r * rand1;
	uint64_t b = ((r * rand2) >> 32) + (uint32_t)a;
	return (uint32_t)(a >> 32) + (uint32_t)(b >> 32);
}
static Vector3 UnitVector3FromSphericalCoordinates(float theta, float phi)
{
	// https://en.wikipedia.org/wiki/Spherical_coordinate_system#Coordinate_system_conversions
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);
	float cosTheta = cosf(theta);
	float sinTheta = sinf(theta);
	return (Vector3)
	{
		cosPhi * sinTheta,
		sinPhi * sinTheta,
		cosTheta
	};
}

unsigned BitNoise1(unsigned seed, int x)
{
	return BitNoise3(seed, x, 0, 0);
}
unsigned BitNoise2(unsigned seed, int x, int y)
{
	return BitNoise3(seed, x, y, 0);
}
unsigned BitNoise3(unsigned seed, int x, int y, int z)
{
	// https://youtu.be/LWFzPP8ZbdU?t=2800.
	// https://nullprogram.com/blog/2018/07/31/
	// http://jonkagstrom.com/tuning-bit-mixers/index.html
	// I randomly cobbled together the magic numbers for this bit mixer.
	// They generate random looking 3D perlin noise, which means they're good enough in my book.
	seed *= 0xB5297A4Du;
	seed ^= seed >> 16;
	seed += (unsigned)x;
	seed *= 0x68E31DA4u;
	seed ^= seed >> 16;
	seed += (unsigned)y;
	seed *= 0x1B56C4E9u;
	seed ^= seed >> 16;
	seed += (unsigned)z;
	seed *= 0x7FEB352Du;
	seed ^= seed >> 16;
	seed *= 0x846CA68Bu;
	seed ^= seed >> 16;
	return seed;
}

int IntNoise1(unsigned seed, int min, int max, int x)
{
	return IntNoise3(seed, min, max, x, 0, 0);
}
int IntNoise2(unsigned seed, int min, int max, int x, int y)
{
	return IntNoise3(seed, min, max, x, y, 0);
}
int IntNoise3(unsigned seed, int min, int max, int x, int y, int z)
{
	ASSERT(min < max);

	unsigned rand1 = BitNoise3(seed + 0x55555555, x, y, z);
	unsigned rand2 = BitNoise3(seed + 0xAAAAAAAA, x, y, z);
	unsigned umin = (unsigned)min;
	unsigned umax = (unsigned)max;
	unsigned range = RandomBitsToRange(rand1, rand2, umax - umin);
	return (int)(umin + range);
}

float FloatNoise1(unsigned seed, int x)
{
	return FloatNoise3(seed, x, 0, 0);
}
float FloatNoise2(unsigned seed, int x, int y)
{
	return FloatNoise3(seed, x, y, 0);
}
float FloatNoise3(unsigned seed, int x, int y, int z)
{
	// https://prng.di.unimi.it/#remarks
	unsigned rand = BitNoise3(seed, x, y, z);
	return (rand >> 8) * 0x1.0p-24f;
}

float ValueNoise1(unsigned seed, float x)
{
	int x0 = (int)floorf(x);
	int x1 = x0 + 1;
	
	float f0 = FloatNoise1(seed, x0) * 2 - 1;
	float f1 = FloatNoise1(seed, x1) * 2 - 1;
	
	float dx = x - (float)x0;
	return f0 + (f1 - f0) * Smootherstep01(dx);
}
float ValueNoise2(unsigned seed, float x, float y)
{
	int x0 = (int)floorf(x);
	int y0 = (int)floorf(y);
	
	int x1 = x0 + 1;
	int y1 = y0 + 1;
	
	float f00 = FloatNoise2(seed, x0, y0) * 2 - 1;
	float f01 = FloatNoise2(seed, x0, y1) * 2 - 1;
	float f10 = FloatNoise2(seed, x1, y0) * 2 - 1;
	float f11 = FloatNoise2(seed, x1, y1) * 2 - 1;
	
	float dx = x - (float)x0;
	float dy = y - (float)y0;

	float weightX = Smootherstep01(dx);
	float weightY = Smootherstep01(dy);

	float f0 = f00 + (f01 - f00) * weightY;
	float f1 = f10 + (f11 - f10) * weightY;
	return f0 + (f1 - f0) * weightX;
}
float ValueNoise3(unsigned seed, float x, float y, float z)
{
	int x0 = (int)floorf(x);
	int y0 = (int)floorf(y);
	int z0 = (int)floorf(z);
	
	int x1 = x0 + 1;
	int y1 = y0 + 1;
	int z1 = z0 + 1;
	
	float f000 = FloatNoise3(seed, x0, y0, z0) * 2 - 1;
	float f001 = FloatNoise3(seed, x0, y0, z1) * 2 - 1;
	float f010 = FloatNoise3(seed, x0, y1, z0) * 2 - 1;
	float f011 = FloatNoise3(seed, x0, y1, z1) * 2 - 1;
	float f100 = FloatNoise3(seed, x1, y0, z0) * 2 - 1;
	float f101 = FloatNoise3(seed, x1, y0, z1) * 2 - 1;
	float f110 = FloatNoise3(seed, x1, y1, z0) * 2 - 1;
	float f111 = FloatNoise3(seed, x1, y1, z1) * 2 - 1;
	
	float dx = x - (float)x0;
	float dy = y - (float)y0;
	float dz = z - (float)z0;

	float weightX = Smootherstep01(dx);
	float weightY = Smootherstep01(dy);
	float weightZ = Smootherstep01(dz);

	float f00 = f000 + (f001 - f000) * weightZ; // We could replace this linear interpolation with Smoothstep/Smootherstep.
	float f01 = f010 + (f011 - f010) * weightZ;
	float f10 = f100 + (f101 - f100) * weightZ;
	float f11 = f110 + (f111 - f110) * weightZ;
	float f0 = f00 + (f01 - f00) * weightY;
	float f1 = f10 + (f11 - f10) * weightY;
	return f0 + (f1 - f0) * weightX;
}
float ValueNoise2V(unsigned seed, Vector2 position)
{
	return ValueNoise2(seed, position.x, position.y);
}
float ValueNoise3V(unsigned seed, Vector3 position)
{
	return ValueNoise3(seed, position.x, position.y, position.z);
}

float PerlinNoise1(unsigned seed, float x)
{
	int x0 = (int)floorf(x);
	int x1 = x0 + 1;

	float dx0 = x - (float)x0;
	float dx1 = x - (float)x1;

	float v0 = FloatNoise1(seed, x0) * 2 - 1;
	float v1 = FloatNoise1(seed, x1) * 2 - 1;
	
	float d0 = dx0 * v0;
	float d1 = dx1 * v1;
	
	return d0 + (d1 - d0) * Smootherstep01(dx0);
}
float PerlinNoise2(unsigned seed, float x, float y)
{
	int x0 = (int)floorf(x);
	int y0 = (int)floorf(y);
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	float dx0 = x - (float)x0;
	float dx1 = x - (float)x1;
	float dy0 = y - (float)y0;
	float dy1 = y - (float)y1;

	Vector2 v00 = UnitVector2WithAngle(2 * PI * FloatNoise2(seed, x0, y0));
	Vector2 v01 = UnitVector2WithAngle(2 * PI * FloatNoise2(seed, x0, y1));
	Vector2 v10 = UnitVector2WithAngle(2 * PI * FloatNoise2(seed, x1, y0));
	Vector2 v11 = UnitVector2WithAngle(2 * PI * FloatNoise2(seed, x1, y1));
	
	float d00 = (dx0 * v00.x) + (dy0 * v00.y);
	float d01 = (dx0 * v01.x) + (dy1 * v01.y);
	float d10 = (dx1 * v10.x) + (dy0 * v10.y);
	float d11 = (dx1 * v11.x) + (dy1 * v11.y);

	float weightY = Smootherstep01(dy0);
	float weightX = Smootherstep01(dx0);

	float d0 = d00 + (d01 - d00) * weightY;
	float d1 = d10 + (d11 - d10) * weightY;
	return d0 + (d1 - d0) * weightX;
}
float PerlinNoise3(unsigned seed, float x, float y, float z)
{
	int x0 = (int)floorf(x);
	int y0 = (int)floorf(y);
	int z0 = (int)floorf(z);
	int x1 = x0 + 1;
	int y1 = y0 + 1;
	int z1 = z0 + 1;

	float dx0 = x - (float)x0;
	float dx1 = x - (float)x1;
	float dy0 = y - (float)y0;
	float dy1 = y - (float)y1;
	float dz0 = z - (float)z0;
	float dz1 = z - (float)z1;

	Vector3 v000 = UnitVector3FromSphericalCoordinates(2 * PI * FloatNoise3(seed, x0, y0, z0), 2 * PI * FloatNoise3(seed + 1, x0, y0, z0));
	Vector3 v001 = UnitVector3FromSphericalCoordinates(2 * PI * FloatNoise3(seed, x0, y0, z1), 2 * PI * FloatNoise3(seed + 1, x0, y0, z1));
	Vector3 v010 = UnitVector3FromSphericalCoordinates(2 * PI * FloatNoise3(seed, x0, y1, z0), 2 * PI * FloatNoise3(seed + 1, x0, y1, z0));
	Vector3 v011 = UnitVector3FromSphericalCoordinates(2 * PI * FloatNoise3(seed, x0, y1, z1), 2 * PI * FloatNoise3(seed + 1, x0, y1, z1));
	Vector3 v100 = UnitVector3FromSphericalCoordinates(2 * PI * FloatNoise3(seed, x1, y0, z0), 2 * PI * FloatNoise3(seed + 1, x1, y0, z0));
	Vector3 v101 = UnitVector3FromSphericalCoordinates(2 * PI * FloatNoise3(seed, x1, y0, z1), 2 * PI * FloatNoise3(seed + 1, x1, y0, z1));
	Vector3 v110 = UnitVector3FromSphericalCoordinates(2 * PI * FloatNoise3(seed, x1, y1, z0), 2 * PI * FloatNoise3(seed + 1, x1, y1, z0));
	Vector3 v111 = UnitVector3FromSphericalCoordinates(2 * PI * FloatNoise3(seed, x1, y1, z1), 2 * PI * FloatNoise3(seed + 1, x1, y1, z1));

	float d000 = (dx0 * v000.x) + (dy0 * v000.y) + (dz0 * v000.z);
	float d001 = (dx0 * v001.x) + (dy0 * v001.y) + (dz1 * v001.z);
	float d010 = (dx0 * v010.x) + (dy1 * v010.y) + (dz0 * v010.z);
	float d011 = (dx0 * v011.x) + (dy1 * v011.y) + (dz1 * v011.z);
	float d100 = (dx1 * v100.x) + (dy0 * v100.y) + (dz0 * v100.z);
	float d101 = (dx1 * v101.x) + (dy0 * v101.y) + (dz1 * v101.z);
	float d110 = (dx1 * v110.x) + (dy1 * v110.y) + (dz0 * v110.z);
	float d111 = (dx1 * v111.x) + (dy1 * v111.y) + (dz1 * v111.z);

	float weightZ = Smootherstep01(dz0);
	float weightY = Smootherstep01(dy0);
	float weightX = Smootherstep01(dx0);

	float d00 = d000 + (d001 - d000) * weightZ;
	float d01 = d010 + (d011 - d010) * weightZ;
	float d10 = d100 + (d101 - d100) * weightZ;
	float d11 = d110 + (d111 - d110) * weightZ;
	float d0 = d00 + (d01 - d00) * weightY;
	float d1 = d10 + (d11 - d10) * weightY;
	return d0 + (d1 - d0) * weightX;
}
float PerlinNoise2V(unsigned seed, Vector2 position)
{
	return PerlinNoise2(seed, position.x, position.y);
}
float PerlinNoise3V(unsigned seed, Vector3 position)
{
	return PerlinNoise3(seed, position.x, position.y, position.z);
}