#ifndef URHOEXTRAS_PROCEDURAL_FUNCTION_HPP
#define URHOEXTRAS_PROCEDURAL_FUNCTION_HPP

#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Container/Vector.h>

#include <cstdint>

namespace UrhoExtras
{

namespace Procedural
{

class Function
{

public:

	inline float get(long x, long y)
	{
		Pos pos(x, y);
		// First try from cache
		Cache::ConstIterator cache_find = cache.Find(pos);
		if (cache_find != cache.End()) {
			return cache_find->second_;
		}

		// Not found from cache. Before calculating value and storing
		// it to cache, check if current cache is too big.
		if (cache.Size() > 1000000) {
			Cache::Iterator i = cache.Begin();
			bool erase_next = true;
			while (i != cache.End()) {
				if (erase_next) {
					i = cache.Erase(i);
				} else {
					++ i;
				}
				erase_next = !erase_next;
			}
		}

		// Calculate and store to cache
		float value = doGet(x, y);
		cache[pos] = value;
		return value;
	}

private:

	struct Pos
	{
		long x, y;
		inline Pos() {}
		inline Pos(long x, long y) : x(x), y(y) {}
		inline bool operator==(Pos const& p) const { return p.x == x && p.y == y; }
		inline unsigned ToHash() const { return x * 31l + y; }
	};

	typedef Urho3D::HashMap<Pos, float> Cache;

	Cache cache;

	virtual float doGet(long x, long y) = 0;
};

}

}

#endif
