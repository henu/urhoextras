#include "statemanager.hpp"

#include <Urho3D/Core/CoreEvents.h>

#include "state.hpp"

namespace UrhoExtras
{

namespace States
{

StateManager::StateManager(Urho3D::Context* context) :
Urho3D::Application(context),
subscribed_to_events(false)
{
}

StateManager::~StateManager()
{
}

void StateManager::pushState(Urho3D::SharedPtr<State> state)
{
	// If there are other states, ask them to go into background.
	if (!stack.Empty()) {
		stack.Back()->hide();
	}

	stack.Push(state);

	state->setStateManager(this);
	state->added();
	state->show();

	if (!subscribed_to_events) {
		SubscribeToEvent(Urho3D::E_ENDFRAME, URHO3D_HANDLER(StateManager, handleEndFrame));
		subscribed_to_events = true;
	}
}

void StateManager::popState()
{
	assert(!stack.Empty());

	// Pop top state
	Urho3D::SharedPtr<State> top_state = stack.Back();
	stack.Pop();

	// Inform State that it no longer belongs to stack
	top_state->hide();
	top_state->removed();
	top_state->setStateManager(NULL);

	// If there are other states, reveal them
	if (!stack.Empty()) {
		stack.Back()->show();
	}
}

void StateManager::handleEndFrame(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	(void)eventType;
	(void)eventData;

	// If there is no more states at the end of the frame, then stop application.
	if (stack.Empty()) {
		engine_->Exit();
	}
}

}

}
