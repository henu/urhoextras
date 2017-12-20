#include "collisions.hpp"

#include <Urho3D/Math/Plane.h>

namespace UrhoExtras
{

inline void nearestPointToLine(Urho3D::Vector3 const& point,
                               Urho3D::Vector3 const& line_pos1, Urho3D::Vector3 const& line_pos2,
                               Urho3D::Vector3* nearest_point, float* m, float* dst_to_point)
{
	Urho3D::Vector3 dir = line_pos2 - line_pos1;
	float dp_rd_rd = dir.DotProduct(dir);
	assert(dp_rd_rd != 0.0);
	float m2 = dir.DotProduct(point - line_pos1) / dp_rd_rd;
	// Store results
	if (m) {
		*m = m2;
	}
	if (nearest_point) {
		*nearest_point = line_pos1 + dir * m2;
		if (dst_to_point) {
			*dst_to_point = (*nearest_point - point).Length();
		}
	} else if (dst_to_point) {
		*dst_to_point = ((line_pos1 + dir * m2) - point).Length();
	}
}

inline float distanceBetweenLines(Urho3D::Vector3 const& begin1, Urho3D::Vector3 const& dir1,
                                  Urho3D::Vector3 const& begin2, Urho3D::Vector3 const& dir2,
                                  Urho3D::Vector3* nearest_point1, Urho3D::Vector3* nearest_point2)
{
	Urho3D::Plane helper_plane(dir1, begin1);

	Urho3D::Vector3 cp_d1_d2 = dir1.CrossProduct(dir2);
	float cp_d1_d2_len_to_2 = cp_d1_d2.LengthSquared();
	if (cp_d1_d2_len_to_2 < 0.000001) {
		Urho3D::Vector3 helper = helper_plane.Project(begin2);
		if (nearest_point1) {
			*nearest_point1 = begin1;
		}
		if (nearest_point2) {
			*nearest_point2 = helper;
		}
		return (begin1 - helper).Length();
	}
	Urho3D::Vector3 begin_diff = begin1 - begin2;
	Urho3D::Vector3 cp = begin_diff.CrossProduct(cp_d1_d2 / cp_d1_d2_len_to_2);

	// Calculate nearest points, if needed
	if (nearest_point1) {
		*nearest_point1 = begin1 + cp.DotProduct(dir2) * dir1;
	}
	if (nearest_point2) {
		*nearest_point2 = begin2 + cp.DotProduct(dir1) * dir2;
	}

	// Calculate distance
	float cp_d1_d2_len = ::sqrt(cp_d1_d2_len_to_2);
	Urho3D::Vector3 n = cp_d1_d2 / cp_d1_d2_len;
	return ::fabs(n.DotProduct(begin_diff));
}

inline Urho3D::Vector2 transformPointToTrianglespace(Urho3D::Vector3 const& pos, Urho3D::Vector3 const& x_axis, Urho3D::Vector3 const& y_axis)
{
	// Calculate helper vector that is in 90 degree against y_axis.
	Urho3D::Vector3 helper = x_axis.CrossProduct(y_axis).CrossProduct(y_axis);
	Urho3D::Vector2 result;

	float dp_xh = x_axis.DotProduct(helper);
	assert(dp_xh != 0);
	result.x_ = pos.DotProduct(helper) / dp_xh;

	Urho3D::Vector3 y_axis_abs(std::fabs(y_axis.x_), std::fabs(y_axis.y_), std::fabs(y_axis.z_));
	if (y_axis_abs.x_ > y_axis_abs.y_ &&
	    y_axis_abs.x_ > y_axis_abs.z_) {
		assert(y_axis.x_ != 0);
		result.y_ = (pos.x_ - result.x_ * x_axis.x_) / y_axis.x_;
	} else if (y_axis_abs.y_ > y_axis_abs.z_) {
		assert(y_axis.y_ != 0);
		result.y_ = (pos.y_ - result.x_ * x_axis.y_) / y_axis.y_;
	} else {
		assert(y_axis.z_ != 0);
		result.y_ = (pos.z_ - result.x_ * x_axis.z_) / y_axis.z_;
	}

	return result;
}


inline bool triangleHitsSphere(Urho3D::Vector3 const& pos, float radius, Triangle const& tri,
                               Urho3D::Vector3& coll_pos, Urho3D::Vector3& coll_nrm, float& coll_depth)
{
	Urho3D::Plane plane = tri.getPlane();

	Urho3D::Vector3 edge0(tri.p2 - tri.p1);
	Urho3D::Vector3 edge1(tri.p3 - tri.p2);

	// Before collision check, do bounding sphere check.
	Urho3D::Vector3 const& tri_bs_pos2 = tri.p2;
	float dst0 = edge0.Length();
	float dst1 = edge1.Length();
	float tri_bs_r2 = (dst0 > dst1) ? dst0 : dst1;
	if ((pos - tri_bs_pos2).Length() > radius + tri_bs_r2) {
		return false;
	}

	// Do the collision check.
	Urho3D::Vector3 edge2(tri.p1 - tri.p3);
	// Form normal of plane. If result is zero, then do not test against
	// plane. Only do edge and corner tests then.
	Urho3D::Vector3 plane_nrm = edge0.CrossProduct(-edge2);
	float plane_nrm_length = plane_nrm.Length();
	if (plane_nrm_length > 0.0) {
		// Check if center of sphere is above/below triangle
		assert(plane_nrm.LengthSquared() > 0.0);
		Urho3D::Vector3 pos_at_plane = plane.Project(pos);

		// We can calculate collision normal and depth here.
		coll_nrm = pos_at_plane - pos;
		coll_depth = radius - coll_nrm.Length();

		// Do not check any other stuff if sphere is too far away from
		// triangle plane.
		if (coll_depth > 0) {

			// Check if the position is inside the area of triangle
			Urho3D::Vector2 pos_at_tri = transformPointToTrianglespace(pos_at_plane - tri.p1, edge0, -edge2);
			if (pos_at_tri.x_ >= 0.0 && pos_at_tri.y_ >= 0.0 && pos_at_tri.x_ + pos_at_tri.y_ <= 1.0) {
				coll_pos = pos_at_plane;
				float coll_nrm_length = coll_nrm.Length();
				if (coll_nrm_length != 0) {
					coll_nrm = -coll_nrm / coll_nrm_length;
				} else {
					coll_nrm = plane_nrm / plane_nrm_length;
				}
				assert(coll_nrm.LengthSquared() != 0.0);
				return true;
			}
		} else {
			return false;
		}
	}

	float deepest_neg_depth = 0;
	bool coll_found;

	// Check if edges collide
	float dp0 = tri.p1.DotProduct(tri.p1);
	float dp1 = tri.p2.DotProduct(tri.p2);
	float dp2 = tri.p3.DotProduct(tri.p3);
	float dp01 = tri.p1.DotProduct(tri.p2);
	float dp12 = tri.p2.DotProduct(tri.p3);
	float dp20 = tri.p3.DotProduct(tri.p1);
	float val0 = tri.p1.DotProduct(pos);
	float val1 = tri.p2.DotProduct(pos);
	float val2 = tri.p3.DotProduct(pos);
	float divider = 2*dp01-dp0-dp1;
	if (divider != 0) {
		Urho3D::Vector3 edge0_np = tri.p1 + edge0 * ((dp01-val1+val0-dp0) / divider);
		float edge0_len = edge0.Length();
		float edge0_dst = (edge0_np - pos).Length();
		if (edge0_dst <= radius &&
		    (tri.p1 - edge0_np).Length() <= edge0_len &&
		    (tri.p2 - edge0_np).Length() <= edge0_len) {
			coll_pos = edge0_np;
			coll_nrm = edge0_np - pos;
			deepest_neg_depth = coll_nrm.Length();
			coll_found = true;
		} else coll_found = false;
	} else coll_found = false;
	divider = 2*dp12-dp1-dp2;
	if (divider != 0) {
		Urho3D::Vector3 edge1_np = tri.p2 + edge1 * ((dp12-val2+val1-dp1) / divider);
		float edge1_len = edge1.Length();
		float edge1_dst = (edge1_np - pos).Length();
		if (edge1_dst <= radius &&
		    (tri.p2 - edge1_np).Length() <= edge1_len &&
		    (tri.p3 - edge1_np).Length() <= edge1_len) {
			if (!coll_found) {
				coll_pos = edge1_np;
				coll_nrm = edge1_np - pos;
				deepest_neg_depth = coll_nrm.Length();
				coll_found = true;
			} else {
				Urho3D::Vector3 test_nrm = edge1_np - pos;
				float test_neg_depth = test_nrm.Length();
				if (test_neg_depth < deepest_neg_depth) {
					coll_pos = edge1_np;
					coll_nrm = test_nrm;
					deepest_neg_depth = test_neg_depth;
				}
			}
		}
	}
	divider = 2*dp20-dp2-dp0;
	if (divider != 0) {
		Urho3D::Vector3 edge2_np = tri.p3 + edge2 * ((dp20-val0+val2-dp2) / divider);
		float edge2_len = edge2.Length();
		float edge2_dst = (edge2_np - pos).Length();
		if (edge2_dst <= radius &&
		    (tri.p3 - edge2_np).Length() <= edge2_len &&
		    (tri.p1 - edge2_np).Length() <= edge2_len) {
			if (!coll_found) {
				coll_pos = edge2_np;
				coll_nrm = edge2_np - pos;
				deepest_neg_depth = coll_nrm.Length();
				coll_found = true;
			} else {
				Urho3D::Vector3 test_nrm = edge2_np - pos;
				float test_neg_depth = test_nrm.Length();
				if (test_neg_depth < deepest_neg_depth) {
					coll_pos = edge2_np;
					coll_nrm = test_nrm;
					deepest_neg_depth = test_neg_depth;
				}
			}
		}
	}

	// Check if corners hit sphere
	if ((tri.p1 - pos).Length() <= radius) {
		if (!coll_found) {
			coll_pos = tri.p1;
			coll_nrm = tri.p1 - pos;
			deepest_neg_depth = coll_nrm.Length();
			coll_found = true;
		} else {
			Urho3D::Vector3 test_nrm = tri.p1 - pos;
			float test_neg_depth = test_nrm.Length();
			if (test_neg_depth < deepest_neg_depth) {
				coll_pos = tri.p1;
				coll_nrm = test_nrm;
				deepest_neg_depth = test_neg_depth;
			}
		}
	}
	if ((tri.p2 - pos).Length() <= radius) {
		if (!coll_found) {
			coll_pos = tri.p2;
			coll_nrm = tri.p2 - pos;
			deepest_neg_depth = coll_nrm.Length();
			coll_found = true;
		} else {
			Urho3D::Vector3 test_nrm = tri.p2 - pos;
			float test_neg_depth = test_nrm.Length();
			if (test_neg_depth < deepest_neg_depth) {
				coll_pos = tri.p2;
				coll_nrm = test_nrm;
				deepest_neg_depth = test_neg_depth;
			}
		}
	}
	if ((tri.p3 - pos).Length() <= radius) {
		if (!coll_found) {
			coll_pos = tri.p3;
			coll_nrm = tri.p3 - pos;
			deepest_neg_depth = coll_nrm.Length();
			coll_found = true;
		} else {
			Urho3D::Vector3 test_nrm = tri.p3 - pos;
			float test_neg_depth = test_nrm.Length();
			if (test_neg_depth < deepest_neg_depth) {
				coll_pos = tri.p3;
				coll_nrm = test_nrm;
				deepest_neg_depth = test_neg_depth;
			}
		}
	}
	if (coll_found) {
		coll_depth = radius - deepest_neg_depth;
		assert(deepest_neg_depth != 0.0);
		coll_nrm /= -deepest_neg_depth;
		return true;
	}
	return false;
}


inline void sphereToTriangle(Collisions& result,
                             Urho3D::Vector3 const& pos, float radius,
                             Triangle const& tri,
                             float extra_radius, bool only_front_collisions)
{
	if (extra_radius < 0) {
		extra_radius = radius;
	}

	Urho3D::Vector3 coll_pos;
	Collision coll;
	if (!triangleHitsSphere(pos, radius + extra_radius, tri, coll_pos, coll.normal, coll.depth)) {
		return;
	}
	assert(coll.normal.Length() > 0.99 && coll.normal.Length() < 1.01);

	// If facing wrong way
	if (only_front_collisions) {
		Urho3D::Plane plane = tri.getPlane();
		if (plane.normal_.DotProduct(coll.normal) < 0) {
			return;
		}
	}

	coll.depth -= extra_radius;
	result.Push(coll);
}

inline void capsuleToTriangle(Collisions& result,
                              Urho3D::Vector3 const& pos0, Urho3D::Vector3 const& pos1, float radius,
                              Triangle const& tri, float extra_radius, bool only_front_collisions)
{
	if (extra_radius < 0) {
		extra_radius = radius;
	}

	Urho3D::Vector3 const diff = pos1 - pos0;

	Urho3D::Plane plane = tri.getPlane();

	// This is kind of hard shape, so go different kind of
	// collision types through and pick all collisions to
	// container. Finally deepest one of these is selected.
	Collisions ccolls;
	ccolls.Reserve(14);

	// Test first capsule cap
	sphereToTriangle(ccolls, pos0, radius, tri, extra_radius, only_front_collisions);

	// Test second capsule cap
	sphereToTriangle(ccolls, pos1, radius, tri, extra_radius, only_front_collisions);

	// Now check middle cylinder. Start from corners
	for (unsigned corner_i = 0; corner_i < 3; ++ corner_i) {
		Urho3D::Vector3 corner = tri.getCorner(corner_i);

		// Discard this corner, if its above or below cylinder
		if (diff.DotProduct(corner - pos0) < 0 ||
		    (-diff).DotProduct(corner - pos1) < 0) {
			continue;
		}

		// Calculate depth
		float distance_to_centerline;
		Urho3D::Vector3 point_at_centerline;
		nearestPointToLine(corner, pos0, pos1, &point_at_centerline, NULL, &distance_to_centerline);
		float depth = radius - distance_to_centerline;

		if (depth + extra_radius > 0) {
			Collision new_ccoll;
			new_ccoll.depth = depth;
			new_ccoll.normal = point_at_centerline - corner;
			float new_ccoll_normal_len = new_ccoll.normal.Length();
			// If division by zero would occure,
			// then just ignore this collision
			if (new_ccoll_normal_len == 0) {
				continue;
			}
			if (!only_front_collisions || plane.normal_.DotProduct(new_ccoll.normal) > 0) {
				new_ccoll.normal /= new_ccoll_normal_len;
				ccolls.Push(new_ccoll);
			}
		}

	}

	// Then check if edges are inside cylinder
	for (unsigned edge_i = 0; edge_i < 3; ++ edge_i) {
		Urho3D::Vector3 begin = tri.getCorner(edge_i);
		Urho3D::Vector3 end = tri.getCorner((edge_i + 1) % 3);
		Urho3D::Vector3 edge = end - begin;

		// Is begin and end of edge inside or outside?
		// 0 = inside, -1 = below, 1 = outside.
		int8_t begin_outside, end_outside;
		if (diff.DotProduct(begin - pos0) < 0) begin_outside = -1;
		else if ((-diff).DotProduct(begin - pos1) < 0) begin_outside = 1;
		else begin_outside = 0;
		if (diff.DotProduct(end - pos0) < 0) end_outside = -1;
		else if ((-diff).DotProduct(end - pos1) < 0) end_outside = 1;
		else end_outside = 0;

		// Discard this edge, if its fully outside cylinder
		if ((begin_outside == -1 && end_outside == -1) ||
		    (begin_outside == 1 && end_outside == 1)) {
			continue;
		}

		// Get collision where edge enters cylinder from below
		if (begin_outside == -1) {
			float dp_d_e = diff.DotProduct(edge);
			if (fabs(dp_d_e) > 0.0005) {
				Urho3D::Vector3 x = begin + edge * (diff.DotProduct(pos0) - diff.DotProduct(begin)) / dp_d_e;
				float depth = radius - x.Length();
				if (depth + extra_radius > 0) {
					Collision new_ccoll;
					new_ccoll.depth = depth;
					new_ccoll.normal = -x.Normalized();
					if (!only_front_collisions || plane.normal_.DotProduct(new_ccoll.normal) > 0) {
						ccolls.Push(new_ccoll);
					}
				}
			}
		}

		// Get collision where edge leaves cylinder from above
		if (end_outside == 1) {
			float dp_d_e = diff.DotProduct(edge);
			if (fabs(dp_d_e) > 0.0005) {
				Urho3D::Vector3 x = end - edge * ((-diff).DotProduct(pos1) - (-diff).DotProduct(end)) / dp_d_e;
				float depth = radius - x.Length();
				if (depth + extra_radius > 0) {
					Collision new_ccoll;
					new_ccoll.depth = depth;
					new_ccoll.normal = -x.Normalized();
					if (!only_front_collisions || plane.normal_.DotProduct(new_ccoll.normal) > 0) {
						ccolls.Push(new_ccoll);
					}
				}
			}
		}

		// Get collision where edge is nearest
		// to the center line of the cylinder
		Urho3D::Vector3 centerline = pos1 - pos0;
		Urho3D::Vector3 point_at_centerline;
		Urho3D::Vector3 point_at_edge;
		float dst = distanceBetweenLines(pos0, centerline, begin, edge, &point_at_centerline, &point_at_edge);
		// Add new collision if:
		// 1) Depth is not too small and
		// 2) nearest point is inside cylinder and at the edge
		float depth = radius - dst;
		if (depth + extra_radius > 0 &&
		    centerline.DotProduct(point_at_centerline - pos0) > 0 &&
		    (-centerline).DotProduct(point_at_centerline - pos1) > 0 &&
		    edge.DotProduct(point_at_edge - begin) > 0 &&
		    (-edge).DotProduct(point_at_edge - end) > 0) {
			Collision new_ccoll;
			new_ccoll.depth = depth;
			new_ccoll.normal = (point_at_centerline - point_at_edge).Normalized();
			if (!only_front_collisions || plane.normal_.DotProduct(new_ccoll.normal) > 0) {
				ccolls.Push(new_ccoll);
			}
		}

		// Get collisions where cylinder hits triangle plane! Are these situations even possible?
// TODO: Code this!

	}

	if (!ccolls.Empty()) {
		// Search the deepest collision
		float deepest_depth = ccolls[0].depth;
		unsigned deepest = 0;
		for (unsigned ccoll_id = 1; ccoll_id < ccolls.Size(); ++ ccoll_id) {
			Collision const& ccoll = ccolls[ccoll_id];
			if (ccoll.depth > deepest_depth) {
				deepest_depth = ccoll.depth;
				deepest = ccoll_id;
			}
		}

		result.Push(ccolls[deepest]);
	}
}

Urho3D::Vector3 moveOutFromCollisions(Collisions& colls)
{
	if (colls.Empty()) {
		return Urho3D::Vector3::ZERO;
	}

	Urho3D::Vector3 result;
	Collisions float_colls;

	// Find out what collisions is the deepest one
	size_t deepest = 0;
	float deepest_depth = -999999;
	for (size_t colls_id = 0;
	     colls_id < colls.Size();
	     colls_id ++) {
		Collision& coll = colls[colls_id];
		assert(coll.normal.LengthSquared() > 0.999 && coll.normal.LengthSquared() < 1.001);
		if (coll.depth > deepest_depth) {
			deepest_depth = coll.depth;
			deepest = colls_id;
		}
	}

	Collision& coll_d = colls[deepest];

	// First move the object out using the deepest collision
	if (deepest_depth >= 0.0) {
		result = coll_d.normal * deepest_depth;
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
	float deepest2_depth = -999999;
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
		float dp_nn_nn = coll.normal.DotProduct(coll.normal);
		assert(dp_nn_nn != 0.0);
		float dp_r_n = result.DotProduct(coll.normal);
		coll.depth -= dp_r_n / dp_nn_nn;

		// Skip collisions that are not touching
		if (coll.depth <= 0.0005) {
			continue;
		}

		// Project normal to the plane of deepest collision. This must
		// be done in two steps. First direction at plane is
		// calculated, and then normal is projected to plane using it.
		float dp_cdnn_cdnn = coll_d.normal.DotProduct(coll_d.normal);
		float dp_cdnn_nn = coll_d.normal.DotProduct(coll.normal);
		assert(dp_cdnn_cdnn != 0);
		Urho3D::Vector3 dir_at_plane = coll.normal - coll_d.normal * (dp_cdnn_nn / dp_cdnn_cdnn);
		float dir_at_plane_len = dir_at_plane.Length();
		if (dir_at_plane_len < 0.0005) {
			continue;
		}
		dir_at_plane /= dir_at_plane_len;
		// Second step
		float dp_n_n = coll.normal.DotProduct(coll.normal) * coll.depth * coll.depth;
		float dp_n_d = coll.normal.DotProduct(dir_at_plane) * coll.depth;
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
	Urho3D::Vector3 move_v = coll_d.normal.CrossProduct(coll_d2.normal);
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
		float dp_nn_nn = coll.normal.DotProduct(coll.normal);
		assert(dp_nn_nn != 0.0);
		float dp_cd2n_n = coll.normal.DotProduct(coll_d2.normal*coll_d2.depth);
		float depthmod = dp_cd2n_n / dp_nn_nn;
		coll.depth -= depthmod;

		// Skip collisions that are not touching
		if (coll.depth <= 0.0005) {
			continue;
		}
		float_colls.Push(coll);

		// Project normal to the move vector. If vectors are in 90°
		// against each others, then this collision must be abandon,
		// because we could never move along move_v to undo this
		// collision.
		Urho3D::Vector3 coll_float = coll.normal * coll.depth;
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

void CollisionShape::getCollisionsToTriangle(Collisions& result, Triangle const& tri, Urho3D::BoundingBox const& bb, float extra_radius, bool only_front_collisions) const
{
// TODO: Use BoundingBox!
(void)bb;
	if (type == SPHERE) {
		sphereToTriangle(result, pos1, radius, tri, extra_radius, only_front_collisions);
	} else {
		capsuleToTriangle(result, pos1, pos2, radius, tri, extra_radius, only_front_collisions);
	}
}

}
