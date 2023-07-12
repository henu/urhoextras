#include "particleplayer.hpp"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

namespace UrhoExtras
{

namespace Graphics
{

ParticlePlayer::ParticlePlayer(Urho3D::Context* context) :
    Urho3D::BillboardSet(context),
    anim_time(0),
    auto_remove(Urho3D::REMOVE_DISABLED)
{
}

void ParticlePlayer::setAnimation(ParticleAnimation* anim)
{
    this->anim = anim;
    anim_time = 0;
    ps_frames.Clear();
    ps_frames.Resize(anim->getParticlesSize(), 0);
    SetNumBillboards(anim->getParticlesSize());
    update(0);
    SetMaterial(anim->getMaterial());
}

void ParticlePlayer::setAutoRemoveMode(Urho3D::AutoRemoveMode mode)
{
    auto_remove = mode;
}

void ParticlePlayer::registerObject(Urho3D::Context* context)
{
    context->RegisterFactory<ParticlePlayer>();
// TODO: Add attributes!
}

void ParticlePlayer::OnSetEnabled()
{
    BillboardSet::OnSetEnabled();

    Urho3D::Scene* scene = GetScene();
    if (scene) {
        if (IsEnabledEffective()) {
            SubscribeToEvent(scene, Urho3D::E_SCENEPOSTUPDATE, URHO3D_HANDLER(ParticlePlayer, handleScenePostUpdate));
        } else {
            UnsubscribeFromEvent(scene, Urho3D::E_SCENEPOSTUPDATE);
        }
    }
}

void ParticlePlayer::OnSceneSet(Urho3D::Scene* scene)
{
    BillboardSet::OnSceneSet(scene);

    if (scene && IsEnabledEffective()) {
        SubscribeToEvent(scene, Urho3D::E_SCENEPOSTUPDATE, URHO3D_HANDLER(ParticlePlayer, handleScenePostUpdate));
    } else if (!scene) {
        UnsubscribeFromEvent(scene, Urho3D::E_SCENEPOSTUPDATE);
    }
}

bool ParticlePlayer::update(float deltatime)
{
    anim_time += deltatime;

    // Iterate all particles
    bool all_finished = true;
    ParticleAnimation::BillboardProperties bb_props;
    for (unsigned p_i = 0; p_i < ps_frames.Size(); ++ p_i) {
        Urho3D::Billboard* bb = GetBillboard(p_i);
        // Do possible frame update
        unsigned p_frame = anim->getParticleFrameNumber(p_i, anim_time, ps_frames[p_i]);
        ps_frames[p_i] = p_frame;
        // Check the state of particle
        ParticleAnimation::ParticleState p_state = anim->getParticleState(p_i, anim_time);
        if (p_state == ParticleAnimation::ACTIVE) {
            bb->enabled_ = true;
            // Update billboard properties
            anim->getParticleBillboardProperties(bb_props, p_i, anim_time, p_frame);
            bb->position_ = bb_props.pos;
            bb->size_ = bb_props.size;
            bb->uv_ = bb_props.uv;
            bb->color_ = bb_props.color;
            bb->rotation_ = bb_props.rot;
        } else {
            bb->enabled_ = false;
        }
        // If this is not yet finished, then keep track that at least one non-finished is found
        if (p_state != ParticleAnimation::FINISHED) {
            all_finished = false;
        }
    }

    Commit();

    return !all_finished;
}

void ParticlePlayer::handleScenePostUpdate(Urho3D::StringHash event_type, Urho3D::VariantMap& event_data)
{
    (void)event_type;
    float deltatime = event_data[Urho3D::ScenePostUpdate::P_TIMESTEP].GetFloat();
    if (!update(deltatime)) {
        DoAutoRemove(auto_remove);
    }
}

}

}
