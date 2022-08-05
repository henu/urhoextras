#ifndef URHOEXTRAS_COLLISIONS_BOX_HPP
#define URHOEXTRAS_COLLISIONS_BOX_HPP

#include "capsule.hpp"
#include "sphere.hpp"
#include "utils.hpp"

#include <Urho3D/Math/Quaternion.h>
#include <stdexcept>

namespace UrhoExtras
{

namespace Collisions
{

class Box : public Shape
{

public:

    inline Box(Urho3D::Vector3 const& size, Urho3D::Vector3 const& pos, Urho3D::Quaternion const& rot = Urho3D::Quaternion::IDENTITY) :
        size_half(size / 2),
        pos(pos),
        rot(rot)
    {
    }

    inline Urho3D::BoundingBox getBoundingBox(float extra_radius = 0) const override
    {
        // Start with a rotated box
        Urho3D::BoundingBox bb(pos - size_half, pos + size_half);
        bb.Transform(rot.RotationMatrix());

        // Then apply extra radius
        Urho3D::Vector3 extra_radius_v3 = Urho3D::Vector3::ONE * extra_radius;
        bb.min_ -= extra_radius_v3;
        bb.max_ += extra_radius_v3;

        return bb;
    }

    inline void getCollisionsTo(Collisions& result, Shape const* other, float extra_radius = 0, bool flip_coll_normal = false) const override
    {
        // If this is a sphere
        Sphere const* sphere = dynamic_cast<Sphere const*>(other);
        if (sphere) {
            // Transform sphere, so its relative to box
            Urho3D::Quaternion rot_inv = rot.Inverse();
            Urho3D::Vector3 sphere_pos = rot_inv * (sphere->getPosition() - pos);
            // Now start collecting collisions
            unsigned result_original_size = result.Size();
            // Box to capsule caps
            getRawSphereCollisionToBox(result, size_half, sphere_pos, sphere->getRadius(), extra_radius, !flip_coll_normal);
            assert(result.Size() <= result_original_size + 1);

            return;
        }

        // If this is a capsule
        Capsule const* capsule = dynamic_cast<Capsule const*>(other);
        if (capsule) {
            // Transform cylinder, so its relative to box
            Urho3D::Quaternion rot_inv = rot.Inverse();
            Urho3D::Vector3 pos1 = rot_inv * (capsule->getPosition1() - pos);
            Urho3D::Vector3 pos2 = rot_inv * (capsule->getPosition2() - pos);
            // Now start collecting collisions
            unsigned result_original_size = result.Size();
            // Box to capsule caps
            getRawSphereCollisionToBox(result, size_half, pos1, capsule->getRadius(), extra_radius, !flip_coll_normal);
            getRawSphereCollisionToBox(result, size_half, pos2, capsule->getRadius(), extra_radius, !flip_coll_normal);
// TODO: Code collision checks of the middle part!
            // Remove all but the deepest collision
            dropAllExceptDeepestCollision(result, result_original_size);

            return;
        }

        // If this is another box
        Box const* box = dynamic_cast<Box const*>(other);
        if (box) {
throw std::runtime_error("Not implemented yet! (3)");
            return;
        }

        // If shape was not recognized, then do the collision check the other way
        other->getCollisionsTo(result, this, extra_radius, !flip_coll_normal);
    }

private:

    Urho3D::Vector3 size_half;
    Urho3D::Vector3 pos;
    Urho3D::Quaternion rot;
};

}

}

#endif
