#ifndef URHOEXTRAS_COLLISIONS_COLLISION_HPP
#define URHOEXTRAS_COLLISIONS_COLLISION_HPP

#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Container/Vector.h>

namespace UrhoExtras
{

namespace Collisions
{

class Collision
{

public:

    inline Collision() :
        depth(0)
    {
    }

    inline Collision(Urho3D::Vector3 const& normal, float depth) :
        normal(normal),
        depth(depth)
    {
        assert(Urho3D::Abs(normal.Length() - 1.0) < Urho3D::M_LARGE_EPSILON);
    }

    inline Urho3D::Vector3 const& getNormal() const
    {
        return normal;
    }

    inline float getDepth() const
    {
        return depth;
    }

    inline void setDepth(float depth)
    {
        this->depth = depth;
    }

private:

    Urho3D::Vector3 normal;
    float depth;
};

typedef Urho3D::Vector<Collision> Collisions;

}

}

#endif
