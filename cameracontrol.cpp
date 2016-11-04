#include "cameracontrol.hpp"

#include <Urho3D/Input/Input.h>
#include <Urho3D/Math/MathDefs.h>

namespace UrhoExtras
{

CameraControl::CameraControl(Urho3D::Context* context) :
Urho3D::Object(context),
key_forward(Urho3D::KEY_W),
key_backward(Urho3D::KEY_S),
key_left(Urho3D::KEY_A),
key_right(Urho3D::KEY_D),
key_jump(Urho3D::KEY_SPACE),
key_crouch(Urho3D::KEY_LCTRL),
yaw_sensitivity(0.1),
pitch_sensitivity(0.1),
pitch(0),
yaw(0),
buttons(0)
{
}

void CameraControl::update()
{
	Urho3D::Input* input = GetSubsystem<Urho3D::Input>();

	buttons = 0;

	if (input->GetKeyDown(key_forward)) buttons |= BUTTON_FORWARD;
	if (input->GetKeyDown(key_backward)) buttons |= BUTTON_BACKWARD;
	if (input->GetKeyDown(key_left)) buttons |= BUTTON_LEFT;
	if (input->GetKeyDown(key_right)) buttons |= BUTTON_RIGHT;
	if (input->GetKeyDown(key_jump)) buttons |= BUTTON_JUMP;
	if (input->GetKeyDown(key_crouch)) buttons |= BUTTON_CROUCH;

	yaw += float(input->GetMouseMoveX()) * yaw_sensitivity;
	pitch = Urho3D::Clamp(pitch + float(input->GetMouseMoveY()) * pitch_sensitivity, -90.0f, 90.0f);
}

void CameraControl::getRotation(Urho3D::Quaternion& result) const
{
	result = Urho3D::Quaternion(pitch, yaw, 0);
}

void CameraControl::getFlyingMovement(Urho3D::Vector3& result) const
{
	result = Urho3D::Vector3::ZERO;

	bool zero = true;

	// Forward and backward
	if ((buttons & BUTTON_FORWARD) && !(buttons & BUTTON_BACKWARD)) {
		result.x_ = Urho3D::Sin(yaw) * Urho3D::Cos(pitch);
		result.z_ = Urho3D::Cos(yaw) * Urho3D::Cos(pitch);
		result.y_ = -Urho3D::Sin(pitch);
		zero = false;
	}
	else if (!(buttons & BUTTON_FORWARD) && (buttons & BUTTON_BACKWARD)) {
		result.x_ = -Urho3D::Sin(yaw) * Urho3D::Cos(pitch);
		result.z_ = -Urho3D::Cos(yaw) * Urho3D::Cos(pitch);
		result.y_ = Urho3D::Sin(pitch);
		zero = false;
	}

	// Left and right
	if ((buttons & BUTTON_LEFT) && !(buttons & BUTTON_RIGHT)) {
		result.x_ -= Urho3D::Cos(yaw) * Urho3D::Cos(pitch);
		result.z_ += Urho3D::Sin(yaw) * Urho3D::Cos(pitch);
		zero = false;
	}
	else if (!(buttons & BUTTON_LEFT) && (buttons & BUTTON_RIGHT)) {
		result.x_ += Urho3D::Cos(yaw) * Urho3D::Cos(pitch);
		result.z_ -= Urho3D::Sin(yaw) * Urho3D::Cos(pitch);
		zero = false;
	}

	// Jump and crouch
	if ((buttons & BUTTON_JUMP) && !(buttons & BUTTON_CROUCH)) {
		result.y_ += 1;
		zero = false;
	}
	else if (!(buttons & BUTTON_JUMP) && (buttons & BUTTON_CROUCH)) {
		result.y_ -= 1;
		zero = false;
	}

	if (!zero) {
		result.Normalize();
	}
}

}
