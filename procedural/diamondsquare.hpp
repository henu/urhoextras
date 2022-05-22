#ifndef URHOEXTRAS_PROCEDURAL_DIAMONDSQUARE_HPP
#define URHOEXTRAS_PROCEDURAL_DIAMONDSQUARE_HPP

#include "function.hpp"
#include "md5rng.hpp"

#include <Urho3D/Math/MathDefs.h>

namespace UrhoExtras
{

namespace Procedural
{

class DiamondSquare : public Function
{

public:

    inline DiamondSquare(unsigned base, unsigned seed) :
    seed(seed),
    base(base)
    {
        base_bits = 0;
        while (base > 1) {
            ++ base_bits;
            base /= 2;
        }
    }
    inline virtual ~DiamondSquare()
    {
    }

private:

    unsigned seed;

    unsigned base;
    unsigned base_bits;

    virtual void doGet(Value& result, long x, long y)
    {
        uint32_t r = md5Rng(seed, x, y);
        float f = double(r) / 0xffffffff;

        // Use bits of coordinates to
        // determine what case this is
        long x_bits = x;
        long y_bits = y;
        for (unsigned bits = 0; bits < base_bits; ++ bits) {
            long step = 1 << bits;
            bool x_bit = x_bits & 1;
            bool y_bit = y_bits & 1;
            // Case of square step
            if (x_bit != y_bit) {
                float north = get(x, y + step);
                float south = get(x, y - step);
                float east = get(x + step, y);
                float west = get(x - step, y);
                result.Push(Urho3D::Clamp<float>((north + south + east + west) / 4 + (f - 0.5) * 2 / (base >> bits), 0, 1));
                return;
            }
            // Case of diamond step
            if (x_bit && y_bit) {
                float ne = get(x + step, y + step);
                float se = get(x + step, y - step);
                float nw = get(x - step, y + step);
                float sw = get(x - step, y - step);
                result.Push(Urho3D::Clamp<float>((ne + se + nw + sw) / 4 + (f - 0.5) * 4 / (base >> bits), 0, 1));
                return;
            }

            x_bits /= 2;
            y_bits /= 2;
        }

        // Case of initial corner values
        result.Push(f);
        return;
    }
};

}

}

#endif
