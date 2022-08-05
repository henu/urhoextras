#ifndef URHOEXTRAS_COLLISIONS_SPHERE_HPP
#define URHOEXTRAS_COLLISIONS_SPHERE_HPP

#include "rawcollisions.hpp"
#include "shape.hpp"

#include <stdexcept>

namespace UrhoExtras
{

namespace Collisions
{

class Sphere : public Shape
{

public:

    inline Sphere(Urho3D::Vector3 const& pos, float radius) :
        pos(pos),
        radius(radius)
    {
    }

    inline Urho3D::Vector3 getPosition() const
    {
        return pos;
    }

    inline float getRadius() const
    {
        return radius;
    }

    inline Urho3D::BoundingBox getBoundingBox(float extra_radius = 0) const override
    {
        return Urho3D::BoundingBox(
            pos - Urho3D::Vector3::ONE * (radius + extra_radius),
            pos + Urho3D::Vector3::ONE * (radius + extra_radius)
        );
    }

    inline void getCollisionsTo(Collisions& result, Shape const* other, float extra_radius = 0, bool flip_coll_normal = false) const override
    {
        // If this is another sphere
        Sphere const* sphere = dynamic_cast<Sphere const*>(other);
        if (sphere) {
            getRawSphereCollisionToSphere(result, pos, sphere->pos, radius + sphere->radius, extra_radius, flip_coll_normal);
        }

        // If shape was not recognized, then do the collision check the other way
        other->getCollisionsTo(result, this, extra_radius, !flip_coll_normal);
    }

private:

    Urho3D::Vector3 pos;
    float radius;
};

}

}

#endif
