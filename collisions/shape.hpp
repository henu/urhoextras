#ifndef URHOEXTRAS_COLLISIONS_SHAPE_HPP
#define URHOEXTRAS_COLLISIONS_SHAPE_HPP

#include "collision.hpp"

#include <Urho3D/Container/RefCounted.h>
#include <Urho3D/Math/BoundingBox.h>

namespace UrhoExtras
{

namespace Collisions
{

class Shape : Urho3D::RefCounted
{

public:

    virtual Urho3D::BoundingBox getBoundingBox(float extra_radius = 0) const = 0;

    // Collision normals will point towards this Shape
    virtual void getCollisionsTo(Collisions& result, Shape const* other, float extra_radius = 0, bool flip_coll_normal = false) const = 0;

private:

};

}

}

#endif
