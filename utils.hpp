#ifndef URHOEXTRAS_UTILS_HPP
#define URHOEXTRAS_UTILS_HPP

#include "mathutils.hpp"

#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>

#include <fstream>
#include <stdexcept>

namespace UrhoExtras
{

inline Urho3D::Quaternion getDirectionalLightRotation(Urho3D::Vector3 const& dir)
{
	Urho3D::Vector2 dir_xz(dir.x_, dir.z_);
	float yaw = getAngle(dir_xz);
	float pitch = getAngle(-dir.y_, dir_xz.Length());
	return Urho3D::Quaternion(yaw, Urho3D::Vector3::UP) * Urho3D::Quaternion(pitch, Urho3D::Vector3::RIGHT);
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

