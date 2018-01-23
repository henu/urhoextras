#ifndef URHOEXTRAS_MATHUTILS_HPP
#define URHOEXTRAS_MATHUTILS_HPP

#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Matrix2.h>

namespace UrhoExtras
{

// Returns distance to plane. If point is at the back side of plane, then
// distance is negative. Note, that distance is measured in length of
// plane_normal, so if you want it to be measured in basic units, then
// normalize plane_normal.
inline float distanceTo2DPlane(Urho3D::Vector2 const& point, Urho3D::Vector2 const& plane_pos, Urho3D::Vector2 const& plane_normal);

// Rotates vector clockwise
inline Urho3D::Vector2 rotateVector2(Urho3D::Vector2 const& v, float angle);

inline float distanceTo2DPlane(Urho3D::Vector2 const& point, Urho3D::Vector2 const& plane_pos, Urho3D::Vector2 const& plane_normal)
{
	float dp_nn = plane_normal.DotProduct(plane_normal);
	return (plane_normal.DotProduct(point) - plane_normal.DotProduct(plane_pos)) / dp_nn;
}

inline Urho3D::Vector2 rotateVector2(Urho3D::Vector2 const& v, float angle)
{
	float s = Urho3D::Sin(angle);
	float c = Urho3D::Cos(angle);
	return Urho3D::Matrix2(c, s, -s, c) * v;
}

}

#endif
