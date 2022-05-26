#ifndef URHOEXTRAS_PROCEDURAL_SCALER_HPP
#define URHOEXTRAS_PROCEDURAL_SCALER_HPP

#include "function.hpp"

#include <Urho3D/Container/Ptr.h>

namespace UrhoExtras
{

namespace Procedural
{

template <class T> class Scaler : public Function<T>
{
    URHO3D_OBJECT(Scaler, UrhoExtras::Procedural::Function<T>);

public:

    inline Scaler(Urho3D::Context* context, Function<T>* func, long begin_x, long begin_y, long end_x, long end_y) :
        UrhoExtras::Procedural::Function<T>(context, begin_x, begin_y, end_x, end_y),
        func(func),
        width(end_x - begin_x),
        height(end_y - begin_y),
        func_width(func->getDataEndX() - func->getDataBeginX()),
        func_height(func->getDataEndY() - func->getDataBeginY())
    {
    }

private:

    Urho3D::SharedPtr<Function<T> > func;

    // Some precalculated values
    unsigned width, height;
    unsigned func_width, func_height;

    inline void doGet(T& result, long x, long y) override
    {
// TODO: Currently this gives quite ugly results. Implement a smooth scaler in the future!
        float x_f_begin = float(x - this->getDataBeginX()) / width;
        float y_f_begin = float(y - this->getDataBeginY()) / height;
        float x_f_end = float(x + 1 - this->getDataBeginX()) / width;
        float y_f_end = float(y + 1 - this->getDataBeginY()) / height;

        long func_x_begin = func->getDataBeginX() + func_width * x_f_begin;
        long func_y_begin = func->getDataBeginY() + func_height * y_f_begin;
        long func_x_end = func->getDataBeginX() + func_width * x_f_end;
        long func_y_end = func->getDataBeginY() + func_height * y_f_end;

        func_x_begin = Urho3D::Clamp(func_x_begin, func->getDataBeginX(), func->getDataEndX() - 1);
        func_y_begin = Urho3D::Clamp(func_y_begin, func->getDataBeginY(), func->getDataEndY() - 1);
        func_x_end = Urho3D::Clamp(func_x_end, func_x_begin + 1, func->getDataEndX());
        func_y_end = Urho3D::Clamp(func_y_end, func_y_begin + 1, func->getDataEndY());
        unsigned sample_width = func_x_end - func_x_begin;
        unsigned sample_height = func_y_end - func_y_begin;

        float weight = 1 / (sample_width * sample_height);
        bool first = true;
        for (long y2 = func_y_begin; y2 < func_y_end; ++ y2) {
            for (long x2 = func_x_begin; x2 < func_x_end; ++ x2) {
                if (first) {
                    result = func->get(x2, y2) * weight;
                    first = false;
                } else {
                    result += func->get(x2, y2) * weight;
                }
            }
        }
    }
};

}

}

#endif
