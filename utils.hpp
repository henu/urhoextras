#ifndef URHOEXTRAS_UTILS_HPP
#define URHOEXTRAS_UTILS_HPP

#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>

namespace UrhoExtras
{

// (0, 1) = 0 deg, (1, 0) = 90 deg, (0, -1) = 180 deg, (-1, 0) = -90 deg
inline float getAngle(float x, float y)
{
	if (y > 0) {
		return Urho3D::Atan(x / y);
	} else if (y < 0) {
		if (x >= 0) {
			return 180 + Urho3D::Atan(x / y);
		}
		return -180 + Urho3D::Atan(x / y);
	} else if (x >= 0) {
		return 90;
	}
	return -90;
}

inline float getAngle(Urho3D::Vector2 const& v)
{
	return getAngle(v.x_, v.y_);
}

inline Urho3D::Quaternion getDirectionalLightRotation(Urho3D::Vector3 const& dir)
{
	Urho3D::Vector2 dir_xz(dir.x_, dir.z_);
	float yaw = getAngle(dir_xz);
	float pitch = getAngle(-dir.y_, dir_xz.Length());
	return Urho3D::Quaternion(yaw, Urho3D::Vector3::UP) * Urho3D::Quaternion(pitch, Urho3D::Vector3::RIGHT);
}

}

#endif

