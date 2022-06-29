#include "text.h"
#include <vector>

extern "C" int TestCpp(void)
{
	std::vector<int> ints;
	for (int i = 0; i < 1000; ++i)
		ints.push_back(i);

	int sum = 0;
	for (auto &x : ints)
		sum += x;

	return sum;
}