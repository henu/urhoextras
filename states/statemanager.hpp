#ifndef URHOEXTRAS_STATES_STATEMANAGER_HPP
#define URHOEXTRAS_STATES_STATEMANAGER_HPP

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Engine/Application.h>

namespace UrhoExtras
{

namespace States
{

class State;

class StateManager : public Urho3D::Application
{
	URHO3D_OBJECT(StateManager, Object)

public:

	StateManager(Urho3D::Context* context);
	virtual ~StateManager();

	void pushState(Urho3D::SharedPtr<State> state);
	void popState();

private:

	typedef Urho3D::Vector<Urho3D::SharedPtr<State> > StateStack;

	void handleEndFrame(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);

	StateStack stack;

	bool subscribed_to_events;
};

}

}

#endif
