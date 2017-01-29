#ifndef URHOEXTRAS_RANDOM_HPP
#define URHOEXTRAS_RANDOM_HPP

namespace UrhoExtras
{

class Random
{

public:

	inline Random() : seed(0) {}
	inline Random(int seed) : seed(seed) {}

	inline int randomInt()
	{
		seed = A * seed + C;
		return seed;
	}

	inline unsigned randomUnsigned()
	{
		return (unsigned)randomInt();
	}

	inline unsigned randomUnsigned(unsigned max_exclusive)
	{
		return randomUnsigned() % max_exclusive;
	}

	inline int randomRange(int min_inclusive, int max_inclusive)
	{
		return min_inclusive + randomUnsigned(max_inclusive - min_inclusive + 1);
	}

	inline float randomFloat()
	{
		return float(randomUnsigned()) / 0xffffffff;
	}

private:

	static unsigned const A = 1103515245;
	static unsigned const C = 12345;

	int seed;

};

}

#endif
