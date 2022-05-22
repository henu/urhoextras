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

    typedef Urho3D::PODVector<float> Value;

    inline void get(Value& result, long x, long y)
    {
        Pos pos(x, y);
        // First try from cache
        Cache::ConstIterator cache_find = cache.Find(pos);
        if (cache_find != cache.End()) {
            result = cache_find->second_;
            return;
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
        result.Clear();
        doGet(result, x, y);
        cache[pos] = result;
    }

    inline float get(long x, long y)
    {
        Pos pos(x, y);
        // First try from cache
        Cache::ConstIterator cache_find = cache.Find(pos);
        if (cache_find != cache.End()) {
            assert(cache_find->second_.Size() == 1);
            return cache_find->second_[0];
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
        Value result;
        doGet(result, x, y);
        assert(result.Size() == 1);
        cache[pos] = result;

        return result[0];
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

    typedef Urho3D::HashMap<Pos, Value> Cache;

    Cache cache;

    // Result is always cleared
    virtual void doGet(Value& result, long x, long y) = 0;
};

}

}

#endif
