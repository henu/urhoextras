#ifndef URHOEXTRAS_COLLISIONS_CAPSULE_HPP
#define URHOEXTRAS_COLLISIONS_CAPSULE_HPP

#include "rawcollisions.hpp"
#include "sphere.hpp"
#include "utils.hpp"

#include <stdexcept>

namespace UrhoExtras
{

namespace Collisions
{

class Capsule : public Shape
{

public:

    inline Capsule(Urho3D::Vector3 const& pos1, Urho3D::Vector3 const& pos2, float radius) :
        pos1(pos1),
        pos2(pos2),
        radius(radius)
    {
    }

    inline Urho3D::Vector3 getPosition1() const
    {
        return pos1;
    }

    inline Urho3D::Vector3 getPosition2() const
    {
        return pos2;
    }

    inline float getRadius() const
    {
        return radius;
    }

    inline Urho3D::BoundingBox getBoundingBox(float extra_radius = 0) const override
    {
        Urho3D::BoundingBox bb(
            pos1 - Urho3D::Vector3::ONE * (radius + extra_radius),
            pos1 + Urho3D::Vector3::ONE * (radius + extra_radius)
        );
        bb.Merge(pos2 - Urho3D::Vector3::ONE * (radius + extra_radius));
        bb.Merge(pos2 + Urho3D::Vector3::ONE * (radius + extra_radius));
        return bb;
    }

    inline void getCollisionsTo(Collisions& result, Shape const* other, float extra_radius = 0, bool flip_coll_normal = false) const override
    {
        // If this is a sphere
        Sphere const* sphere = dynamic_cast<Sphere const*>(other);
        if (sphere) {
            unsigned result_original_size = result.Size();
            // Check both spherical ends of the capsule
            float both_radiuses_sum = radius + sphere->getRadius();
            getRawSphereCollisionToSphere(result, pos1, sphere->getPosition(), both_radiuses_sum, extra_radius, flip_coll_normal);
            getRawSphereCollisionToSphere(result, pos2, sphere->getPosition(), both_radiuses_sum, extra_radius, flip_coll_normal);
            // Check cylinderical middle part
// TODO: Code this!
            // Remove all but the deepest collision
            dropAllExceptDeepestCollision(result, result_original_size);

            return;
        }

        // If this is another capsule
        Capsule const* capsule = dynamic_cast<Capsule const*>(other);
        if (capsule) {
throw std::runtime_error("Not implemented yet! (4)");
            return;
        }

        // If shape was not recognized, then do the collision check the other way
        other->getCollisionsTo(result, this, extra_radius, !flip_coll_normal);
    }

private:

    Urho3D::Vector3 pos1, pos2;
    float radius;
};

}

}

#endif
