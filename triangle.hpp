#ifndef URHOEXTRAS_TRIANGLE_HPP
#define URHOEXTRAS_TRIANGLE_HPP

#include <Urho3D/Math/Matrix4.h>
#include <Urho3D/Math/Plane.h>
#include <Urho3D/Math/Vector3.h>

namespace UrhoExtras
{

class Triangle
{

public:

	inline Triangle() {}
	inline Triangle(Urho3D::Vector3 const& p1, Urho3D::Vector3 const& p2, Urho3D::Vector3 const& p3) :
	p1(p1), p2(p2), p3(p3) {}

	inline Urho3D::Plane getPlane() const { return Urho3D::Plane(p1, p2, p3); }

	inline Urho3D::Vector3 getCorner(unsigned i) const
	{
		if (i == 0) return p1;
		if (i == 1) return p2;
		return p3;
	}

	Urho3D::Vector3 p1, p2, p3;
};

inline Triangle operator*(Urho3D::Matrix4 const& m, Triangle const& t)
{
	return Triangle(m * t.p1, m * t.p2, m * t.p3);
}

}

#endif
