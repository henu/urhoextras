#ifndef URHOEXTRAS_STATES_STATE_HPP
#define URHOEXTRAS_STATES_STATE_HPP

#include "statemanager.hpp"

#include <Urho3D/Core/Object.h>

namespace UrhoExtras
{

namespace States
{

class State : public Urho3D::Object
{
	URHO3D_OBJECT(State, Object)

public:

	inline State(Urho3D::Context* context) : Urho3D::Object(context) {}
	inline virtual ~State() {}

	// Called when added to StateManasger
	inline virtual void added() {}

	// Called when restored or after added
	inline virtual void show() {}

	// called when put to background or before removed
	inline virtual void hide() {}

	// Called when removed from StateManager
	inline virtual void removed() {}

	inline void setStateManager(StateManager* statemanager) { this->statemanager = statemanager; }
	inline StateManager* getStateManager() const { return statemanager; }

private:

	StateManager* statemanager;

};

}

}

#endif
