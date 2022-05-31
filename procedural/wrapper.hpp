#ifndef URHOEXTRAS_PROCEDURAL_WRAPPER_HPP
#define URHOEXTRAS_PROCEDURAL_WRAPPER_HPP

#include "function.hpp"

namespace UrhoExtras
{

namespace Procedural
{

class Wrapper : public Function<float>
{
    URHO3D_OBJECT(Wrapper, UrhoExtras::Procedural::Function<float>);

public:

    inline Wrapper(Urho3D::Context* context, long begin_x, long begin_y, long end_x, long end_y) :
        UrhoExtras::Procedural::Function<float>(context, begin_x, begin_y, end_x, end_y),
        width(end_x - begin_x)
    {
    }

    template<typename T>
    inline void setData(Urho3D::PODVector<T> data, T min, T max)
    {
        float range = float(max - min);
        unsigned offset = 0;
        for (long y = getDataBeginY(); y < getDataEndY(); ++ y) {
            for (long x = getDataBeginX(); x < getDataEndX(); ++ x) {
                float val = float(data[offset] - min) / range;
                set(x, y, val);
                ++ offset;
            }
        }
    }

private:

    unsigned width;

    inline void doGet(float& result, long x, long y) override
    {
        // This should be never called
        (void)result;
        (void)x;
        (void)y;
        throw std::runtime_error("Data is not set!");
    }
};

}

}

#endif
