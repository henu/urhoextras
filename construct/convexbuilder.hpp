#ifndef URHOEXTRAS_CONSTRUCT_CONVEXBUILDER_HPP
#define URHOEXTRAS_CONSTRUCT_CONVEXBUILDER_HPP

#include "../modelcombiner.hpp"

#include <Urho3D/Container/RefCounted.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Math/Plane.h>

namespace UrhoExtras
{

namespace Construct
{

class ConvexBuilder : public Urho3D::RefCounted
{

public:

    ConvexBuilder(ModelCombiner* combiner, float uv_scaling = 1);

    // NULL material means cutting a hole
    void addPlane(Urho3D::Plane const& plane, Urho3D::Material* mat);

    void finish();

private:

    struct Plane
    {
        Urho3D::Plane plane;
        Urho3D::Material* mat;
        inline Plane(Urho3D::Plane const& plane, Urho3D::Material* mat) :
            plane(plane),
            mat(mat)
        {
        }
    };
    typedef Urho3D::PODVector<Plane> Planes;

    ModelCombiner* combiner;

    float uv_scaling;

    Planes planes;
};

}

}

#endif
