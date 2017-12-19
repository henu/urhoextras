#ifndef URHOEXTRAS_TRIANGLE_HPP
#define URHOEXTRAS_TRIANGLE_HPP

#include <Urho3D/Math/Vector3.h>

namespace UrhoExtras
{

class Triangle
{

public:

	inline Triangle() {}
	inline Triangle(Urho3D::Vector3 const& p1, Urho3D::Vector3 const& p2, Urho3D::Vector3 const& p3) :
	p1(p1), p2(p2), p3(p3) {}

	Urho3D::Vector3 p1, p2, p3;
};

}

#endif
