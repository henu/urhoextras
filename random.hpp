#ifndef URHOEXTRAS_RANDOM_HPP
#define URHOEXTRAS_RANDOM_HPP

#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>

namespace UrhoExtras
{

class Random
{

public:

	inline Random() : seed(0) {}
	inline Random(int seed) : seed(seed) {}

	inline void seedMore(int new_seed)
	{
		seed = seed * A + new_seed * C;
	}

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

	inline bool randomBool()
	{
		return randomUnsigned() % 2;
	}

	inline float randomFloat()
	{
		return float(randomUnsigned()) / 0xffffffff;
	}

	inline float randomFloatRange(float min_inclusive, float max_inclusive)
	{
		return min_inclusive + randomFloat() * (max_inclusive - min_inclusive);
	}

	inline Urho3D::Vector2 randomVector2(float max_radius_inclusive)
	{
		if (max_radius_inclusive <= 0) {
			return Urho3D::Vector2::ZERO;
		}
		while (true) {
			Urho3D::Vector2 result(
				randomFloatRange(-max_radius_inclusive, max_radius_inclusive),
				randomFloatRange(-max_radius_inclusive, max_radius_inclusive)
			);
			if (result.Length() <= max_radius_inclusive) {
				return result;
			}
		}
	}

	inline Urho3D::Vector3 randomVector3(float max_radius_inclusive)
	{
		if (max_radius_inclusive <= 0) {
			return Urho3D::Vector3::ZERO;
		}
		while (true) {
			Urho3D::Vector3 result(
				randomFloatRange(-max_radius_inclusive, max_radius_inclusive),
				randomFloatRange(-max_radius_inclusive, max_radius_inclusive),
				randomFloatRange(-max_radius_inclusive, max_radius_inclusive)
			);
			if (result.Length() <= max_radius_inclusive) {
				return result;
			}
		}
	}

private:

	static unsigned const A = 1103515245;
	static unsigned const C = 12345;

	int seed;

};

}

#endif
