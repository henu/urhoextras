#ifndef URHOEXTRAS_GRAPHICS_PARTICLEANIMATION_HPP
#define URHOEXTRAS_GRAPHICS_PARTICLEANIMATION_HPP

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/Resource.h>

namespace UrhoExtras
{

namespace Graphics
{

class ParticleAnimation : public Urho3D::Resource
{
    URHO3D_OBJECT(ParticleAnimation, Urho3D::Resource);

public:

    enum ParticleState
    {
        WAITING,
        ACTIVE,
        FINISHED
    };

    struct BillboardProperties
    {
        Urho3D::Vector3 pos;
        Urho3D::Vector2 size;
        Urho3D::Rect uv;
        Urho3D::Color color;
        float rot;
    };

    ParticleAnimation(Urho3D::Context* context);

    bool BeginLoad(Urho3D::Deserializer& source) override;
    bool EndLoad() override;

    Urho3D::Material* getMaterial();

    unsigned getParticlesSize() const;

    unsigned getParticleFrameNumber(unsigned p_i, float anim_time, unsigned current_frame) const;

    ParticleState getParticleState(unsigned p_i, float anim_time) const;

    void getParticleBillboardProperties(BillboardProperties& result, unsigned p_i, float anim_time, unsigned current_frame) const;

    static void registerObject(Urho3D::Context* context);

private:

    struct ParticleFrame
    {
        BillboardProperties bb_props;
        // Animation properties
        float time;
        // For sorting
        inline bool operator<(ParticleFrame const& frame) const
        {
            return time < frame.time;
        }
    };
    typedef Urho3D::Vector<ParticleFrame> ParticleFrames;

    typedef Urho3D::Vector<ParticleFrames> Particles;

    Urho3D::String mat_name;
    Urho3D::SharedPtr<Urho3D::Material> mat;

    Particles ps;

    bool loadBinaryFile(Urho3D::Deserializer& source);

    bool loadJsonFile(Urho3D::Deserializer& source);
};

}

}

#endif
