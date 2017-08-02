#ifndef URHOEXTRAS_PROCEDURAL_SMOOTH_HPP
#define URHOEXTRAS_PROCEDURAL_SMOOTH_HPP

#include "function.hpp"

namespace UrhoExtras
{

namespace Procedural
{

class Smooth : public Function
{

public:

	inline Smooth(unsigned radius, Function* func) :
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
		for (Cells::Iterator i = cells.Begin(); i != cells.End(); ++ i) {
			i->weight = weight;
		}
	}

private:

	struct Cell
	{
		int x, y;
		float weight;
	};
	typedef Urho3D::Vector<Cell> Cells;

	Function* func;

	Cells cells;

	virtual float doGet(long x, long y)
	{
		float result = 0;
		for (Cells::ConstIterator i = cells.Begin(); i != cells.End(); ++ i) {
			Cell const& c = *i;
			result += c.weight * func->get(x + long(c.x), y + long(c.y));
		}
		return result;
	}
};

}

}

#endif

