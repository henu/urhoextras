#ifndef URHOEXTRAS_COLLISIONS_UTILS_HPP
#define URHOEXTRAS_COLLISIONS_UTILS_HPP

#include "collision.hpp"

namespace UrhoExtras
{

namespace Collisions
{

inline void dropAllExceptDeepestCollision(Collisions& colls, unsigned offset)
{
    if (offset < colls.Size()) {
        // Find deepest collision
        float deepest_depth = colls[offset].getDepth();
        int deepest_offset = offset;
        for (unsigned i = offset + 1; i < colls.Size(); ++ i) {
            Collision const& coll = colls[i];
            if (coll.getDepth() > deepest_depth) {
                deepest_depth = coll.getDepth();
                deepest_offset = i;
            }
        }
        // Drop other collisions after it
        colls.Erase(deepest_offset + 1);
        // Drop other collisions before it
        colls.Erase(offset, deepest_offset - offset);
    }
}

inline void dropAllExceptShallowestCollision(Collisions& colls, unsigned offset)
{
    if (offset < colls.Size()) {
        // Find shallowest collision
        float shallowest_depth = colls[offset].getDepth();
        int shallowest_offset = offset;
        for (unsigned i = offset + 1; i < colls.Size(); ++ i) {
            Collision const& coll = colls[i];
            if (coll.getDepth() < shallowest_depth) {
                shallowest_depth = coll.getDepth();
                shallowest_offset = i;
            }
        }
        // Drop other collisions after it
        colls.Erase(shallowest_offset + 1);
        // Drop other collisions before it
        colls.Erase(offset, shallowest_offset - offset);
    }
}

// Calculates position delta to get object out of walls. Note, that this function invalidates
// depth values of collisions and removes those collisions that does not really hit walls.
inline Urho3D::Vector3 moveOutFromCollisions(Collisions& colls)
{
    if (colls.Empty()) {
        return Urho3D::Vector3::ZERO;
    }

    Urho3D::Vector3 result;
    Collisions float_colls;

    // Find out what collisions is the deepest one
    size_t deepest = 0;
    float deepest_depth = -Urho3D::M_INFINITY;
    for (size_t colls_id = 0;
         colls_id < colls.Size();
         colls_id ++) {
        Collision& coll = colls[colls_id];
        assert(coll.getNormal().LengthSquared() > 0.999 && coll.getNormal().LengthSquared() < 1.001);
        if (coll.getDepth() > deepest_depth) {
            deepest_depth = coll.getDepth();
            deepest = colls_id;
        }
    }

    Collision& coll_d = colls[deepest];

    // First move the object out using the deepest collision
    if (deepest_depth >= 0.0) {
        result = coll_d.getNormal() * deepest_depth;
        float_colls.Push(coll_d);
    } else {
        colls.Clear();
        return Urho3D::Vector3::ZERO;
    }

    // If this was the only collision, then do nothing else
    if (colls.Size() == 1) {
        return result;
    }

    // Go rest of collisions through and check how much they should be
    // moved so object would come out of wall. Note, that since one
    // collision is already fixed, all other movings must be done at the
    // plane of that collision!
    size_t deepest2 = 0;
    float deepest2_depth = -Urho3D::M_INFINITY;
    Urho3D::Vector3 deepest2_nrm_p = Urho3D::Vector3::ZERO;
    for (size_t colls_id = 0;
         colls_id < colls.Size();
         colls_id ++) {
        // Skip deepest collision
        if (colls_id == deepest) {
            continue;
        }
        Collision& coll = colls[colls_id];

        // Since object has already moved, depth of other collisions
        // have changed. Recalculate depth now.
        float dp_nn_nn = coll.getNormal().DotProduct(coll.getNormal());
        assert(dp_nn_nn != 0.0);
        float dp_r_n = result.DotProduct(coll.getNormal());
        coll.setDepth(coll.getDepth() - dp_r_n / dp_nn_nn);

        // Skip collisions that are not touching
        if (coll.getDepth() <= 0.0005) {
            continue;
        }

        // Project normal to the plane of deepest collision. This must
        // be done in two steps. First direction at plane is
        // calculated, and then normal is projected to plane using it.
        float dp_cdnn_cdnn = coll_d.getNormal().DotProduct(coll_d.getNormal());
        float dp_cdnn_nn = coll_d.getNormal().DotProduct(coll.getNormal());
        assert(dp_cdnn_cdnn != 0);
        Urho3D::Vector3 dir_at_plane = coll.getNormal() - coll_d.getNormal() * (dp_cdnn_nn / dp_cdnn_cdnn);
        float dir_at_plane_len = dir_at_plane.Length();
        if (dir_at_plane_len < 0.0005) {
            continue;
        }
        dir_at_plane /= dir_at_plane_len;
        // Second step
        float dp_n_n = coll.getNormal().DotProduct(coll.getNormal()) * coll.getDepth() * coll.getDepth();
        float dp_n_d = coll.getNormal().DotProduct(dir_at_plane) * coll.getDepth();
        if (fabs(dp_n_d) < 0.0005) {
            continue;
        }
        Urho3D::Vector3 move_at_plane = dir_at_plane * (dp_n_n / dp_n_d);

        float depth = move_at_plane.Length();
        if (depth > deepest2_depth) {
            deepest2_depth = depth;
            deepest2 = colls_id;
            deepest2_nrm_p = move_at_plane;
        }
    }

    // If no touching collisions were found, then mark all except deepest
    // as not float and leave.
    if (deepest2_depth <= 0.0) {
        colls.Swap(float_colls);
        return result;
    }

    Collision& coll_d2 = colls[deepest2];
    float_colls.Push(coll_d2);

    // Again, modify position using the second deepest collision
    assert(fabs(deepest2_nrm_p.DotProduct(result)) < 0.005);
    result += deepest2_nrm_p;

    // Go rest of collisions through and check how much they should be
    // moved so object would come out of wall. Note, that since two
    // collisions are already fixed, all other movings must be done at the
    // planes of these collisions! This means one line in space.
    Urho3D::Vector3 move_v = coll_d.getNormal().CrossProduct(coll_d2.getNormal());
    assert(move_v.LengthSquared() != 0.0);
    move_v.Normalize();
    float deepest3_depth = -99999;
    Urho3D::Vector3 deepest3_move = Urho3D::Vector3::ZERO;
    for (size_t colls_id = 0;
         colls_id < colls.Size();
         colls_id ++) {
        // Skip deepest collisions
        if (colls_id == deepest ||
            colls_id == deepest2) {
            continue;
        }
        Collision& coll = colls[colls_id];

        // Since object has moved again, depth of other collisions
        // have changed. Recalculate depth now.
        float dp_nn_nn = coll.getNormal().DotProduct(coll.getNormal());
        assert(dp_nn_nn != 0.0);
        float dp_cd2n_n = coll.getNormal().DotProduct(coll_d2.getNormal()*coll_d2.getDepth());
        float depthmod = dp_cd2n_n / dp_nn_nn;
        coll.setDepth(coll.getDepth() - depthmod);

        // Skip collisions that are not touching
        if (coll.getDepth() <= 0.0005) {
            continue;
        }
        float_colls.Push(coll);

        // Project normal to the move vector. If vectors are in 90°
        // against each others, then this collision must be abandon,
        // because we could never move along move_v to undo this
        // collision.
        Urho3D::Vector3 coll_float = coll.getNormal() * coll.getDepth();
        float dp_c_mv = coll_float.DotProduct(move_v);
        if (fabs(dp_c_mv) > 0.0005) {
            float dp_c_c = coll_float.DotProduct(coll_float);
            Urho3D::Vector3 projected = move_v * (dp_c_c / dp_c_mv);
            if (projected.Length() > deepest3_depth) {
                deepest3_depth = projected.Length();
                deepest3_move = projected;
            }
        }
    }

    // Now move position for final time
    if (deepest3_depth > 0.0) {
        result += deepest3_move;
    }

    colls.Swap(float_colls);
    return result;
}

}

}

#endif
