#ifndef URHOEXTRAS_CAMERACONTROL_HPP
#define URHOEXTRAS_CAMERACONTROL_HPP

#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/InputConstants.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Vector3.h>

namespace UrhoExtras
{

class CameraControl : public Urho3D::Object
{

	URHO3D_OBJECT(CameraControl, Urho3D::Object)

public:

	static unsigned int const BUTTON_FORWARD = 0x01;
	static unsigned int const BUTTON_BACKWARD = 0x02;
	static unsigned int const BUTTON_LEFT = 0x04;
	static unsigned int const BUTTON_RIGHT = 0x08;
	static unsigned int const BUTTON_JUMP = 0x10;
	static unsigned int const BUTTON_CROUCH = 0x20;

	CameraControl(Urho3D::Context* context);

	inline void setPitch(float pitch) { this->pitch = pitch; }
	inline void setYaw(float yaw) { this->yaw = yaw; }

	void update(bool update_rotation = true);

	void getRotation(Urho3D::Quaternion& result) const;
	Urho3D::Quaternion getRotation() const;

	void getFlyingMovement(Urho3D::Vector3& result) const;
	Urho3D::Vector3 getFlyingMovement() const;

private:

	Urho3D::Key key_forward;
	Urho3D::Key key_backward;
	Urho3D::Key key_left;
	Urho3D::Key key_right;
	Urho3D::Key key_jump;
	Urho3D::Key key_crouch;

	float yaw_sensitivity, pitch_sensitivity;

	float pitch, yaw;

	unsigned int buttons;
};

}

#endif
