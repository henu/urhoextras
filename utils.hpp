#ifndef URHOEXTRAS_UTILS_HPP
#define URHOEXTRAS_UTILS_HPP

#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>

#include <fstream>
#include <stdexcept>

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

// Returns a vector that is perpendicular to given one
inline Urho3D::Vector3 getPerpendicular(Urho3D::Vector3 const& v)
{
	if (Urho3D::Abs(v.x_) < Urho3D::Abs(v.y_)) {
		return Urho3D::Vector3(0, v.z_, -v.y_);
	} else {
		return Urho3D::Vector3(-v.z_, 0, v.x_);
	}
}

inline unsigned secureRand()
{
	unsigned result;
	std::ifstream file("/dev/urandom");
	if (!file.is_open()) {
		throw std::runtime_error("Unable to open RNG!");
	}
	file.read((char*)&result, 4);
	return result;
}

}

#endif

