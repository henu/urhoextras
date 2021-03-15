#include "convexbuilder.hpp"

#include "../mathutils.hpp"

#include <Urho3D/Graphics/VertexBuffer.h>

namespace UrhoExtras
{

namespace Construct
{

enum BoundsState
{
    NOT_STARTED,
    STARTED_BUT_NO_LOOP,
    LOOP_DONE
};

inline void calculateUvMapping(Urho3D::Vector3& result_uv_mapping_x, Urho3D::Vector3& result_uv_mapping_y, Urho3D::Vector3 const& normal)
{
    // Decide mapping vectors. Prefer keeping positive Y facing up.
    Urho3D::Vector2 normal_xz(normal.x_, normal.z_);
    float normal_xz_len = normal_xz.Length();
    Urho3D::Vector2 normal_xz_norm;
    if (normal_xz_len > Urho3D::M_LARGE_EPSILON) normal_xz_norm = normal_xz / normal_xz_len;
    else normal_xz_norm = Urho3D::Vector2::UP;
    result_uv_mapping_y = Urho3D::Vector3(normal_xz_norm.x_ * normal.y_, -normal_xz_len, normal_xz_norm.y_ * normal.y_);
    result_uv_mapping_x = result_uv_mapping_y.CrossProduct(normal);
}

ConvexBuilder::ConvexBuilder(ModelCombiner* combiner, float uv_scaling) :
    combiner(combiner),
    uv_scaling(uv_scaling)
{
}

void ConvexBuilder::addPlane(Urho3D::Plane const& plane, Urho3D::Material* mat)
{
    // Prevent duplicate planes
    for (Plane const& plane2 : planes) {
        // If plane values are near enough, then consider it as duplicate
        if (Urho3D::Equals(plane2.plane.d_, plane.d_) && plane2.plane.normal_.Equals(plane.normal_)) {
            return;
        }
    }
    // Add plane
    planes.Push(Plane(plane, mat));
}

void ConvexBuilder::finish()
{
    // These can be reused through the process
    Urho3D::PODVector<Urho3D::Vector3> bounds;
    Urho3D::PODVector<Urho3D::Vector3> new_bounds;
    Urho3D::PODVector<float> distances;

    // Define what components there are for vertices
    Urho3D::PODVector<Urho3D::VertexElement> elems;
    elems.Push(Urho3D::VertexElement(Urho3D::TYPE_VECTOR3, Urho3D::SEM_POSITION));
    elems.Push(Urho3D::VertexElement(Urho3D::TYPE_VECTOR3, Urho3D::SEM_NORMAL));
    elems.Push(Urho3D::VertexElement(Urho3D::TYPE_VECTOR2, Urho3D::SEM_TEXCOORD));
    Urho3D::VertexBuffer::UpdateOffsets(elems);

    // Create faces from all planes
    for (Plane const& plane : planes) {
        // If plane has no material, then it will leave a hole
        if (!plane.mat) {
            continue;
        }

        Urho3D::Vector3 plane_uv_origin = plane.plane.Project(Urho3D::Vector3::ZERO);
        Urho3D::Vector3 plane_uv_mapping_x, plane_uv_mapping_y;
        calculateUvMapping(plane_uv_mapping_x, plane_uv_mapping_y, plane.plane.normal_);

        // Face will be formed from bounds that are defined by other planes
        bounds.Clear();
        // Start construction by forming a very big square bounds. Then use other planes to cut it
        // smaller. This technique will fail if the resulting convex is too big, but it can do for now.
        Urho3D::Vector3 point_on_face = plane.plane.normal_ * -plane.plane.d_;
        assert(Urho3D::Abs(plane.plane.Distance(point_on_face) - 0.0f) < Urho3D::M_EPSILON);
        Urho3D::Vector3 local_x_on_face = getPerpendicular(plane.plane.normal_).Normalized();
        Urho3D::Vector3 local_y_on_face = -plane.plane.normal_.CrossProduct(local_x_on_face);
        bounds.Push(point_on_face + local_x_on_face * 99.9f);
        bounds.Push(point_on_face - local_y_on_face * 99.9f);
        bounds.Push(point_on_face - local_x_on_face * 99.9f);
        bounds.Push(point_on_face + local_y_on_face * 99.9f);
        assert((bounds[0] - point_on_face).CrossProduct(bounds[1] - point_on_face).Angle(plane.plane.normal_) < 5);

        // Go all other planes through and let them cut the bounds smaller
        for (Plane const& plane2 : planes) {
            // Don't compare plane to itself
            if (&plane == &plane2) {
                continue;
            }

            // Find on which side the vertices of the bounds are of this plane. At the
            // same time keep track if all or none of the edges are on the other side.
            bool bound_vrts_at_front = false;
            bool bound_vrts_at_back = false;
            distances.Clear();
            for (Urho3D::Vector3 const& vrt : bounds) {
                float distance = plane2.plane.Distance(vrt);
                if (distance < -Urho3D::M_EPSILON) {
                    bound_vrts_at_back = true;
                } else if (distance > Urho3D::M_EPSILON) {
                    bound_vrts_at_front = true;
                }
                distances.Push(distance);
            }

            // If all vertices are on the back side, then it means this plane doesn't make any cuts to the bounds
            if (!bound_vrts_at_front && bound_vrts_at_back) {
                continue;
            }

            // If all vertices are on the front side, then it means this plane will completely cut out
            // this face. This also handles the unexpected situation that all vertices are on the plane.
            if (!bound_vrts_at_back) {
                if (!bound_vrts_at_front) {
                    URHO3D_LOGWARNING("Found no bounding vertices from front nor from back of plane!");
                }
                bounds.Clear();
                break;
            }

            // Find the edges that go through the plane. There should be two of those
            int edge_starting_from_back = -1;
            int edge_starting_from_front = -1;
            for (unsigned i = 0; i < bounds.Size(); ++ i) {
                unsigned i_next = (i + 1) % distances.Size();
                if (distances[i] < 0 && distances[i_next] >= -Urho3D::M_EPSILON) {
                    if (edge_starting_from_back >= 0) {
                        URHO3D_LOGWARNING("Found two bounding edges starting from back!");
                    }
                    edge_starting_from_back = i;
                    if (edge_starting_from_front >= 0) {
                        break;
                    }
                } else if (distances[i] > 0 && distances[i_next] <= Urho3D::M_EPSILON) {
                    if (edge_starting_from_front >= 0) {
                        URHO3D_LOGWARNING("Found two bounding edges starting from front!");
                    }
                    edge_starting_from_front = i;
                    if (edge_starting_from_back >= 0) {
                        break;
                    }
                }
            }
            // If it failed
            if (edge_starting_from_back < 0 || edge_starting_from_front < 0) {
                URHO3D_LOGWARNING("Not enough bounding edges going through the plane!");
                continue;
            }

            // Find out properties of new edge
            Urho3D::Vector3 edge_starting_from_back_begin = bounds[edge_starting_from_back];
            Urho3D::Vector3 edge_starting_from_back_end = bounds[(edge_starting_from_back + 1) % bounds.Size()];
            Urho3D::Vector3 edge_starting_from_front_begin = bounds[edge_starting_from_front];
            Urho3D::Vector3 edge_starting_from_front_end = bounds[(edge_starting_from_front + 1) % bounds.Size()];
            Urho3D::Vector3 new_edge_begin = projectToPlaneWithDirection(edge_starting_from_back_begin, plane2.plane, edge_starting_from_back_end - edge_starting_from_back_begin);
            Urho3D::Vector3 new_edge_end = projectToPlaneWithDirection(edge_starting_from_front_begin, plane2.plane, edge_starting_from_front_end - edge_starting_from_front_begin);
            assert(Urho3D::Abs(plane2.plane.Distance(new_edge_begin)) < 0.01f);
            assert(Urho3D::Abs(plane2.plane.Distance(new_edge_end)) < 0.01f);
            assert((edge_starting_from_back_begin - new_edge_begin).DotProduct(plane2.plane.normal_) < Urho3D::M_EPSILON);
            // Construct new bounds
            new_bounds.Clear();
            new_bounds.Push(edge_starting_from_back_begin);
            new_bounds.Push(new_edge_begin);
            new_bounds.Push(new_edge_end);
            for (int i = (edge_starting_from_front + 1) % bounds.Size(); i != edge_starting_from_back; i = (i + 1) % bounds.Size()) {
                assert((bounds[i] - new_edge_begin).DotProduct(plane2.plane.normal_) < Urho3D::M_EPSILON);
                new_bounds.Push(bounds[i]);
            }
            bounds.Swap(new_bounds);
        }

        // If there are no bounds left, then this face is not visible
        if (bounds.Size() < 3) {
            if (!bounds.Empty()) {
                URHO3D_LOGWARNING("Ended up with bounds with less than three vertices!");
            }
            continue;
        }

        // Create a face here
        while (bounds.Size() >= 3) {
            // Select vertex that has the smallest angle
            float smallest_angle = 999;
            unsigned smallest_angle_vrt = -1;
            for (unsigned vrt = 0; vrt < bounds.Size(); ++ vrt) {
                Urho3D::Vector3 const& vrt_pos = bounds[vrt];
                Urho3D::Vector3 const& vrt_next_pos = bounds[(vrt + 1) % bounds.Size()];
                Urho3D::Vector3 const& vrt_prev_pos = bounds[(vrt + bounds.Size() - 1) % bounds.Size()];
                float angle = (vrt_next_pos - vrt_pos).Angle(vrt_prev_pos - vrt_pos);
                if (angle < smallest_angle) {
                    smallest_angle = angle;
                    smallest_angle_vrt = vrt;
                }
            }
            // Get vertex properties
            Urho3D::Vector3 const& vrt_pos = bounds[smallest_angle_vrt];
            Urho3D::Vector3 const& vrt_next_pos = bounds[(smallest_angle_vrt + 1) % bounds.Size()];
            Urho3D::Vector3 const& vrt_prev_pos = bounds[(smallest_angle_vrt + bounds.Size() - 1) % bounds.Size()];
            Urho3D::Vector2 vrt_uv = UrhoExtras::transformPointToTrianglespace(vrt_pos - plane_uv_origin, plane_uv_mapping_x, plane_uv_mapping_y) * uv_scaling;
            Urho3D::Vector2 vrt_next_uv = UrhoExtras::transformPointToTrianglespace(vrt_next_pos - plane_uv_origin, plane_uv_mapping_x, plane_uv_mapping_y) * uv_scaling;
            Urho3D::Vector2 vrt_prev_uv = UrhoExtras::transformPointToTrianglespace(vrt_prev_pos - plane_uv_origin, plane_uv_mapping_x, plane_uv_mapping_y) * uv_scaling;
            // Create a new triangle
            combiner->StartAddingTriangle(elems, plane.mat);
            combiner->AddTriangleData(vrt_prev_pos);
            combiner->AddTriangleData(plane.plane.normal_);
            combiner->AddTriangleData(vrt_prev_uv);
            combiner->AddTriangleData(vrt_pos);
            combiner->AddTriangleData(plane.plane.normal_);
            combiner->AddTriangleData(vrt_uv);
            combiner->AddTriangleData(vrt_next_pos);
            combiner->AddTriangleData(plane.plane.normal_);
            combiner->AddTriangleData(vrt_next_uv);
            // Remove the vertex that is now behind the new triangle
            bounds.Erase(smallest_angle_vrt);
        }
    }
}

}

}
