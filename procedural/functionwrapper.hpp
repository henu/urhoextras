#ifndef URHOEXTRAS_PROCEDURAL_FUNCTIONWRAPPER_HPP
#define URHOEXTRAS_PROCEDURAL_FUNCTIONWRAPPER_HPP

#include "function.hpp"

namespace UrhoExtras
{

namespace Procedural
{

class FunctionWrapper : public Function
{

public:

	inline FunctionWrapper(float (*wrapped)(float), Function* func) :
	wrapped(wrapped),
	func(func)
	{
	}

private:

	float (*wrapped)(float);
	Function* func;

	virtual float doGet(long x, long y)
	{
		return wrapped(func->get(x, y));
	}
};

}

}

#endif

