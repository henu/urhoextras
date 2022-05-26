#ifndef URHOEXTRAS_PROCEDURAL_SMOOTH_HPP
#define URHOEXTRAS_PROCEDURAL_SMOOTH_HPP

#include "function.hpp"

#include <Urho3D/Container/Ptr.h>

namespace UrhoExtras
{

namespace Procedural
{

template <class T> class Smooth : public Function<T>
{
    URHO3D_OBJECT(Smooth, UrhoExtras::Procedural::Function<T>);

public:

    inline Smooth(Urho3D::Context* context, unsigned radius, Function<T>* func) :
        UrhoExtras::Procedural::Function<T>(context, func->getDataBeginX(), func->getDataBeginY(), func->getDataEndX(), func->getDataEndY()),
        radius(radius),
        func(func)
    {
        unsigned radius_to_2 = radius * radius;
        for (int y = -radius; y <= int(radius); ++ y) {
            unsigned y_to_2 = y*y;
            for (int x = -radius; x <= int(radius); ++ x) {
                if (y_to_2 + x*x <= radius_to_2) {
                    Cell c;
                    c.x = x;
                    c.y = y;
                    cells.Push(c);
                }
            }
        }
        float weight = 1.0 / cells.Size();
        for (Cell& cell : cells) {
            cell.weight = weight;
        }
    }
    inline virtual ~Smooth()
    {
    }

private:

    struct Cell
    {
        int x, y;
        float weight;
    };
    typedef Urho3D::PODVector<Cell> Cells;

    unsigned radius;

    Urho3D::SharedPtr<Function<T> > func;

    Cells cells;

    inline void doGet(T& result, long x, long y) override
    {
        // If sample is got from the middle of the area, this quicker way can be used
        if (x >= this->getDataBeginX() + radius && y >= this->getDataBeginY() + radius && x < this->getDataEndX() - radius && y < this->getDataEndY() - radius) {
            T cell_value;
            bool first = true;
            for (Cell const& cell : cells) {
                long x2 = x + long(cell.x);
                long y2 = y + long(cell.y);
                assert(x2 >= func->getDataBeginX());
                assert(x2 < func->getDataEndX());
                assert(y2 >= func->getDataBeginY());
                assert(y2 < func->getDataEndY());
                func->get(cell_value, x2, y2);
                if (first) {
                    result = cell_value * cell.weight;
                    first = false;
                } else {
                    result += cell_value * cell.weight;
                }
            }
            return;
        }

        T cell_value;
        bool first = true;
        float total_weight = 0;
        for (Cell const& cell : cells) {
            long x2 = x + long(cell.x);
            long y2 = y + long(cell.y);
            if (x2 >= this->getDataBeginX() && y2 >= this->getDataBeginY() && x2 < this->getDataEndX() && y2 < this->getDataEndY()) {
                func->get(cell_value, x2, y2);
                if (first) {
                    result = cell_value * cell.weight;
                    first = false;
                } else {
                    result += cell_value * cell.weight;
                }
                total_weight += cell.weight;
            }
        }
        result = result * (1 / total_weight);
    }
};

}

}

#endif

