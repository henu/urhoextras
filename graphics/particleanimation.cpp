#include "particleanimation.hpp"

#include "../json.hpp"

#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace UrhoExtras
{

namespace Graphics
{

ParticleAnimation::ParticleAnimation(Urho3D::Context* context) :
    Urho3D::Resource(context)
{
}

bool ParticleAnimation::BeginLoad(Urho3D::Deserializer& source)
{
    Urho3D::JSONFile json_file(context_);
    if (!json_file.Load(source)) {
        URHO3D_LOGERROR("Unable to load Wallmaterial, because JSON file contains errors!");
        return false;
    }
    Urho3D::JSONValue json = json_file.GetRoot();

    // Material
    mat_name = getJsonString(json, "material", "ParticleAnimation material: ");
    Urho3D::ResourceCache* resources = GetSubsystem<Urho3D::ResourceCache>();
    resources->BackgroundLoadResource<Urho3D::Material>(mat_name);

    // Particles
    Urho3D::JSONArray particles_json = getJsonArray(json, "particles", "ParticleAnimation particles: ", 1);
    for (Urho3D::JSONValue const& particle_json : particles_json) {
        if (!particle_json.IsArray()) {
            URHO3D_LOGERROR("ParticleAnimation particle definion must be an array of frames!");
            return false;
        }
        Urho3D::JSONArray const& particle_array = particle_json.GetArray();
        if (particle_array.Size() >= 2) {
            // Read frames of single particle
            ParticleFrames frames;
            for (Urho3D::JSONValue const& frame_json : particle_array) {
                // Read single frame
                ParticleFrame frame;
                frame.bb_props.pos = getJsonVector3(frame_json, "pos", "ParticleAnimation particle frame pos: ");
                frame.bb_props.size = getJsonVector2(frame_json, "size", "ParticleAnimation particle frame size: ");
                frame.bb_props.uv = Urho3D::Rect(getJsonVector4(frame_json, "uv", "ParticleAnimation particle frame uv: "));
                frame.bb_props.color = getJsonColor(frame_json, "color", "ParticleAnimation particle frame color: ");
                frame.bb_props.rot = getJsonFloat(frame_json, "rot", "ParticleAnimation particle frame rot: ");
                frame.time = getJsonFloat(frame_json, "time", "ParticleAnimation particle frame time: ");
                frames.Push(frame);
            }
            // Sort frames, based on their time
            Urho3D::Sort(frames.Begin(), frames.End());

            ps.Push(frames);
        }
    }

    return true;
}

bool ParticleAnimation::EndLoad()
{
    Urho3D::ResourceCache* resources = GetSubsystem<Urho3D::ResourceCache>();

    // Material
    mat = resources->GetResource<Urho3D::Material>(mat_name);
    if (!mat) {
        URHO3D_LOGERROR("Wallmaterial has material that could not be loaded!");
        return false;
    }

    return true;
}

Urho3D::Material* ParticleAnimation::getMaterial()
{
    return mat;
}

unsigned ParticleAnimation::getParticlesSize() const
{
    return ps.Size();
}

unsigned ParticleAnimation::getParticleFrameNumber(unsigned p_i, float anim_time, unsigned current_frame) const
{
    ParticleFrames const& frames = ps[p_i];
    current_frame = Urho3D::Min(current_frame, frames.Size() - 1);
    while (true) {
        // If current frame is actually in the future
        if (frames[current_frame].time > anim_time) {
            if (current_frame > 0) {
                -- current_frame;
            } else {
                break;
            }
        }
        // If this is the last frame
        else if (current_frame == frames.Size() - 1) {
            break;
        }
        // If next frame is in the future
        else if (frames[current_frame + 1].time > anim_time) {
            break;
        }
        // If next frame is already active
        else {
            ++ current_frame;
        }
    }
    return current_frame;
}

ParticleAnimation::ParticleState ParticleAnimation::getParticleState(unsigned p_i, float anim_time) const
{
    ParticleFrames const& frames = ps[p_i];
    if (frames.Front().time > anim_time) {
        return WAITING;
    } else if (frames.Back().time <= anim_time) {
        return FINISHED;
    }
    return ACTIVE;
}

void ParticleAnimation::getParticleBillboardProperties(BillboardProperties& result, unsigned p_i, float anim_time, unsigned current_frame) const
{
    ParticleFrames const& frames = ps[p_i];
    if (current_frame >= frames.Size() - 1) {
        result = frames.Back().bb_props;
        return;
    }
    // Get two frames to interpolate between
    ParticleFrame const& frame1 = frames[current_frame];
    ParticleFrame const& frame2 = frames[current_frame + 1];
    float frame1_len = frame2.time - frame1.time;
    if (frame1_len <= Urho3D::M_EPSILON) {
        result = frame1.bb_props;
        return;
    }
    // Do the interpolation
    float interpolation = (anim_time - frame1.time) / frame1_len;
    BillboardProperties const& bb_props1 = frame1.bb_props;
    BillboardProperties const& bb_props2 = frame2.bb_props;
    result.pos = Urho3D::Lerp(bb_props1.pos, bb_props2.pos, interpolation);
    result.size = Urho3D::Lerp(bb_props1.size, bb_props2.size, interpolation);
    result.color = Urho3D::Lerp(bb_props1.color, bb_props2.color, interpolation);
    result.rot = Urho3D::Lerp(bb_props1.rot, bb_props2.rot, interpolation);
    // UV is assumed to be a "sprite" in a collection of sprites, so it cannot be interpolated.
    result.uv = bb_props1.uv;
}

void ParticleAnimation::registerObject(Urho3D::Context* context)
{
    context->RegisterFactory<ParticleAnimation>();
}

}

}
