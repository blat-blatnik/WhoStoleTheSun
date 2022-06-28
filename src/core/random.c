#include "../core.h"

Random TimeSeededRandom(void)
{
	union
	{
		double f;
		uint64_t i;
	} floatInt;
	
	floatInt.f = GetTime();
	uint64_t x = floatInt.i;

	unsigned seed = (unsigned)x ^ (unsigned)(x >> 32);
	return (Random) { seed };
}

unsigned RandomBits(Random *rand)
{
	return BitNoise1(rand->seed, rand->index++);
}

int RandomInt(Random *rand, int min, int max)
{
	return IntNoise1(rand->seed, min, max, rand->index++);
}

float RandomFloat(Random *rand, float min, float max)
{
	return min + RandomFloat01(rand) * (max - min);
}

float RandomFloat01(Random *rand)
{
	return FloatNoise1(rand->seed, rand->index++);
}

float RandomNormal(Random *rand, float mean, float sdev)
{
	return mean + sdev * RandomNormal01(rand);
}

float RandomNormal01(Random *rand)
{
	// https://en.wikipedia.org/wiki/Marsaglia_polar_method
	float x, y, r;
	do
	{
		x = RandomFloat(rand, -1, +1);
		y = RandomFloat(rand, -1, +1);
		r = x * x + y * y;
	} while (r >= 1 or r == 0);
	return x * sqrtf(-2 * logf(r) / r);
}

bool RandomBool(Random *rand)
{
	return (RandomBits(rand) & 1) != 0;
}

bool RandomProbability(Random *rand, float probabilityOfTrue)
{
	return RandomFloat01(rand) < probabilityOfTrue;
}

int RandomSelect(Random *rand, const float probabilityWeights[], int numWeights)
{
	ASSERT(probabilityWeights and numWeights >= 1);

	float sum = 0;
	for (int i = 0; i < numWeights; ++i)
		sum += probabilityWeights[i];

	float x = sum * RandomFloat01(rand);
	sum = 0;
	for (int i = 0; i < numWeights; ++i) {
		sum += probabilityWeights[i];
		if (x < sum)
			return i;
	}

	// We can reach here due to floating point error. There's sometimes a little bit of leftover probability.
	// In that case just pick a random index. We could also just return the last index, or the index with the highest weight.
	return RandomInt(rand, 0, numWeights);
}

void RandomShuffle(Random *rand, void *items, int numItems, int sizeOfOneItem)
{
	// https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle#The_modern_algorithm
	for (int i = 0; i <= numItems - 2; ++i)
	{
		int j = RandomInt(rand, i, numItems);
		SwapBytes((char *)items + i * sizeOfOneItem, (char *)items + j * sizeOfOneItem, sizeOfOneItem);
	}
}