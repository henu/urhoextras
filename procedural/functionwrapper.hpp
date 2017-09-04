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
	wrapped_single(wrapped),
	wrapped_multi(NULL),
	func(func)
	{
	}

	inline FunctionWrapper(void (*wrapped)(Value& result, Value const& input), Function* func) :
	wrapped_single(NULL),
	wrapped_multi(wrapped),
	func(func)
	{
	}

private:

	float (*wrapped_single)(float);
	void (*wrapped_multi)(Value& result, Value const& input);
	Function* func;

	virtual void doGet(Value& result, long x, long y)
	{
		if (wrapped_multi) {
			Value input;
			func->get(input, x, y);
			wrapped_multi(result, input);
		} else {
			result.Push(wrapped_single(func->get(x, y)));
		}
	}
};

}

}

#endif

