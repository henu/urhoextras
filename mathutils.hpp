#ifndef URHOEXTRAS_MATHUTILS_HPP
#define URHOEXTRAS_MATHUTILS_HPP

#include <Urho3D/IO/Log.h>
#include <Urho3D/Math/Plane.h>
#include <Urho3D/Math/Matrix2.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <cassert>

namespace UrhoExtras
{

// Returns distance to plane. If point is at the back side of plane, then
// distance is negative. Note, that distance is measured in length of
// plane_normal, so if you want it to be measured in basic units, then
// normalize plane_normal.
inline float distanceTo2DPlane(Urho3D::Vector2 const& point, Urho3D::Vector2 const& plane_pos, Urho3D::Vector2 const& plane_normal);

// Rotates vector clockwise
inline Urho3D::Vector2 rotateVector2(Urho3D::Vector2 const& v, float angle);

// Transforms a 3D or 2D point at the plane of triangle to the space of it so
// that the point can be defined using axes of triangle. This can be used to
// check if point is inside triangle or not. Note, that position is measured so
// that triangle vertex where axes start from, is origin.
inline Urho3D::Vector2 transformPointToTrianglespace(Urho3D::Vector3 const& pos, Urho3D::Vector3 const& x_axis, Urho3D::Vector3 const& y_axis);

// (0, 1) = 0 deg, (1, 0) = 90 deg, (0, -1) = 180 deg, (-1, 0) = -90 deg
inline float getAngle(float x, float y);
inline float getAngle(Urho3D::Vector2 const& v);

// (0, 1, 0) = pitch -90 deg
// (0, -1, 0) = pitch 90 deg
// (0, 0, 1) = pitch 0 deg, yaw 0 deg,
// (1, 0, 0) = pitch 0 deg, yaw 90 deg,
// (0, 0, -1) = pitch 0 deg, yaw 180 deg,
// (-1, 0, 0) = pitch 0 deg, yaw -90 deg,
inline void getPitchAndYaw(float& result_pitch, float& result_yaw, Urho3D::Vector3 const& v);

// Converts angle, so it's [-180 - 180]
inline float fixAngle(float angle);

// Returns a vector that is perpendicular to given one
inline Urho3D::Vector3 getPerpendicular(Urho3D::Vector3 const& v);

// Forces angle between vectors to be 90°.
inline void forceVectorsPerpendicular(Urho3D::Vector3& v1, Urho3D::Vector3& v2);

// Calculates nearest point between line/ray and a point. It is possible to get
// the nearest point in two ways and it is possible to get the distance between
// nearest and given points. The nearest point is defined in these ways:
// nearest_point = line_pos1 + (line_pos2 - line_pos1)
// nearest_point = ray_begin + ray_dir * m
// Note, that these functions assume that line is infinite long from both ends
// but ray is infinite only from head!
inline void nearestPointToLine(Urho3D::Vector3 const& point,
                               Urho3D::Vector3 const& line_pos1, Urho3D::Vector3 const& line_pos2,
                               Urho3D::Vector3* nearest_point = nullptr, float* m = nullptr, float* dst_to_point = nullptr);
inline void nearestPointToLine(Urho3D::Vector2 const& point,
                               Urho3D::Vector2 const& line_pos1, Urho3D::Vector2 const& line_pos2,
                               Urho3D::Vector2* nearest_point = nullptr, float* m = nullptr, float* dst_to_point = nullptr);

// Returns distance between two infinite lines
inline float distanceBetweenLines(Urho3D::Vector3 const& begin1, Urho3D::Vector3 const& dir1,
                                  Urho3D::Vector3 const& begin2, Urho3D::Vector3 const& dir2,
                                  Urho3D::Vector3* nearest_point1, Urho3D::Vector3* nearest_point2);

// Get collision point of two infinite lines, or return false if there is not a single such point
inline bool linesCollisionPoint(Urho3D::Vector2& result,
                                Urho3D::Vector2 const& begin1, Urho3D::Vector2 const& end1,
                                Urho3D::Vector2 const& begin2, Urho3D::Vector2 const& end2);

// Project vector to another, by using a shearing method.
// This means vector "v" will never go smaller, but will
// either grow or stay the same length.
inline Urho3D::Vector3 shearVectorToAnother(Urho3D::Vector3 const& v, Urho3D::Vector3 const& another);

inline Urho3D::Vector3 projectToPlaneWithDirection(Urho3D::Vector3 const& pos, Urho3D::Plane const& plane, Urho3D::Vector3 const& projection_dir);

inline float distanceTo2DPlane(Urho3D::Vector2 const& point, Urho3D::Vector2 const& plane_pos, Urho3D::Vector2 const& plane_normal)
{
	float dp_nn = plane_normal.DotProduct(plane_normal);
	return (plane_normal.DotProduct(point) - plane_normal.DotProduct(plane_pos)) / dp_nn;
}

inline Urho3D::Vector2 rotateVector2(Urho3D::Vector2 const& v, float angle)
{
	float s = Urho3D::Sin(angle);
	float c = Urho3D::Cos(angle);
	return Urho3D::Matrix2(c, s, -s, c) * v;
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

inline float getAngle(float x, float y)
{
	if (y > 0) {
		return Urho3D::Atan(x / y);
	} else if (y < 0) {
		if (x >= 0) {
			return 180 + Urho3D::Atan(x / y);
		}
		return -180 + Urho3D::Atan(x / y);
	} else if (x >= 0) {
		return 90;
	}
	return -90;
}

inline float getAngle(Urho3D::Vector2 const& v)
{
	return getAngle(v.x_, v.y_);
}

inline void getPitchAndYaw(float& result_pitch, float& result_yaw, Urho3D::Vector3 const& v)
{
	Urho3D::Vector2 v_xz(v.x_, v.z_);
	result_pitch = getAngle(-v.y_,  v_xz.Length());
	result_yaw = getAngle(v_xz);
}

inline float fixAngle(float angle)
{
    if (angle < -180.0f) {
        angle += 360 * Urho3D::CeilToInt(angle / -360.0f);
    }
    return Urho3D::Mod(angle + 180.0f, 360.0f) - 180.0f;
}

inline Urho3D::Vector3 getPerpendicular(Urho3D::Vector3 const& v)
{
	if (Urho3D::Abs(v.x_) < Urho3D::Abs(v.y_)) {
		return Urho3D::Vector3(0, v.z_, -v.y_);
	} else {
		return Urho3D::Vector3(-v.z_, 0, v.x_);
	}
}

inline void forceVectorsPerpendicular(Urho3D::Vector3& v1, Urho3D::Vector3& v2)
{
	Urho3D::Vector3 center = (v1 + v2) / 2.0;
	Urho3D::Vector3 to_v1 = v1 - center;
	Urho3D::Vector3 to_v2 = v2 - center;
	float dp_tov1_tov2 = to_v1.DotProduct(to_v2);
	float dp_tov1_c = to_v1.DotProduct(center);
	float dp_tov2_c = to_v2.DotProduct(center);
	float dp_c_c = center.DotProduct(center);
	float a = dp_tov1_tov2;
	float b = dp_tov1_c + dp_tov2_c;
	float c = dp_c_c;
	if (fabs(a) < Urho3D::M_EPSILON) {
		URHO3D_LOGERRORF("Unable to force vectors (%s) and (%s) perpendicular! Reason: Division by zero.", v1.ToString().CString(), v2.ToString().CString());
	}
	float d = b*b - 4*a*c;
	if (d < 0) {
		URHO3D_LOGERRORF("Unable to force vectors (%s) and (%s) perpendicular! Reason: Cannot take square from negative number.", v1.ToString().CString(), v2.ToString().CString());
	}
	float sqrt_result = sqrt(d);
	float m1 = (-b + sqrt_result) / (2.0 * a);
	float m2 = (-b - sqrt_result) / (2.0 * a);
	if (m1 > 0.0) {
		v1 = center + m1 * to_v1;
		v2 = center + m1 * to_v2;
	} else {
		if (m2 <= 0.0) {
			URHO3D_LOGERRORF("Unable to force vectors (%s) and (%s) perpendicular! Reason: Impossible situation.", v1.ToString().CString(), v2.ToString().CString());
			return;
		}
		v1 = center + m2 * to_v1;
		v2 = center + m2 * to_v2;
	}
}

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

inline void nearestPointToLine(Urho3D::Vector2 const& point,
                               Urho3D::Vector2 const& line_pos1, Urho3D::Vector2 const& line_pos2,
                               Urho3D::Vector2* nearest_point, float* m, float* dst_to_point)
{
	Urho3D::Vector2 dir = line_pos2 - line_pos1;
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

inline bool linesCollisionPoint(Urho3D::Vector2& result,
                                Urho3D::Vector2 const& begin1, Urho3D::Vector2 const& end1,
                                Urho3D::Vector2 const& begin2, Urho3D::Vector2 const& end2)
{
    float subdet_line1_x = begin1.x_ - end1.x_;
    float subdet_line1_y = begin1.y_ - end1.y_;
    float subdet_line2_x = begin2.x_ - end2.x_;
    float subdet_line2_y = begin2.y_ - end2.y_;
    float det_divider = subdet_line1_x * subdet_line2_y - subdet_line1_y * subdet_line2_x;
    if (det_divider < Urho3D::M_LARGE_EPSILON) {
        return false;
    }
    float subdet_line1 = begin1.x_ * end1.y_ - end1.x_ * begin1.y_;
    float subdet_line2 = begin2.x_ * end2.y_ - end2.x_ * begin2.y_;
    result.x_ = (subdet_line1 * subdet_line2_x - subdet_line2 * subdet_line1_x) / det_divider;
    result.y_ = (subdet_line1 * subdet_line2_y - subdet_line2 * subdet_line1_y) / det_divider;
    return true;
}

inline Urho3D::Vector3 shearVectorToAnother(Urho3D::Vector3 const& v, Urho3D::Vector3 const& another)
{
	float dp_v_a = v.DotProduct(another);
	assert(fabs(dp_v_a) > 0.000001);
	float m = v.DotProduct(v) / dp_v_a;
	return another * m;
}

inline Urho3D::Vector3 projectToPlaneWithDirection(Urho3D::Vector3 const& pos, Urho3D::Plane const& plane, Urho3D::Vector3 const& projection_dir)
{
	float dp_n_d = plane.normal_.DotProduct(projection_dir);
	assert(!Urho3D::Equals(dp_n_d, 0.0f));
	float m = (plane.normal_.DotProduct(plane.normal_) * -plane.d_ - pos.DotProduct(plane.normal_)) / dp_n_d;
	return pos + projection_dir * m;
}

}

#endif
