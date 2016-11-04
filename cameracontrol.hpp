#ifndef URHOEXTRAS_CAMERACONTROL_HPP
#define URHOEXTRAS_CAMERACONTROL_HPP

#include <Urho3D/Core/Object.h>
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

	void update();

	void getRotation(Urho3D::Quaternion& result) const;

	void getFlyingMovement(Urho3D::Vector3& result) const;

private:

	int key_forward;
	int key_backward;
	int key_left;
	int key_right;
	int key_jump;
	int key_crouch;

	float yaw_sensitivity, pitch_sensitivity;

	float pitch, yaw;

	unsigned int buttons;
};

}

#endif
