#ifndef URHOEXTRAS_GRAPHICS_PARTICLEPLAYER_HPP
#define URHOEXTRAS_GRAPHICS_PARTICLEPLAYER_HPP

#include "particleanimation.hpp"

#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/Material.h>

namespace UrhoExtras
{

namespace Graphics
{

class ParticlePlayer : public Urho3D::BillboardSet
{
    URHO3D_OBJECT(ParticlePlayer, Urho3D::BillboardSet);

public:

    ParticlePlayer(Urho3D::Context* context);

    void setAnimation(ParticleAnimation* anim);

    void setAnimationSpeed(float speed);

    void setAutoRemoveMode(Urho3D::AutoRemoveMode mode);

    static void registerObject(Urho3D::Context* context);

    void OnSetEnabled() override;

protected:

    void OnSceneSet(Urho3D::Scene* scene) override;

private:

    typedef Urho3D::Vector<unsigned> ParticleFrames;

    Urho3D::SharedPtr<ParticleAnimation> anim;

    // Animation state
    float anim_time;
    ParticleFrames ps_frames;

    float anim_speed;

    Urho3D::AutoRemoveMode auto_remove;

    // Returns false if animation is complete
    bool update(float deltatime);

    // Handle Scene post update and play animations
    void handleScenePostUpdate(Urho3D::StringHash event_type, Urho3D::VariantMap& event_data);
};

}

}

#endif
