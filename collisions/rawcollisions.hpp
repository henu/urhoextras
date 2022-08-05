#ifndef URHOEXTRAS_COLLISIONS_RAWCOLLISIONS_HPP
#define URHOEXTRAS_COLLISIONS_RAWCOLLISIONS_HPP

#include "collision.hpp"
#include "utils.hpp"
#include "../mathutils.hpp"

namespace UrhoExtras
{

namespace Collisions
{

inline void getRawSphereCollisionToSphere(Collisions& result, Urho3D::Vector3 const& pos, Urho3D::Vector3 const& other_pos, float both_radiuses_sum, float extra_radius, bool flip_coll_normal)
{
    Urho3D::Vector3 diff = pos - other_pos;
    float dst = diff.Length();
    if (dst < both_radiuses_sum + extra_radius) {
        if (flip_coll_normal) {
            diff = -diff;
        }
        result.Push(Collision(diff / dst, both_radiuses_sum - dst));
    }
}

inline void getRawSphereCollisionToLine(Collisions& result, Urho3D::Vector3 const& pos, float radius, Urho3D::Vector3 const& pos1, Urho3D::Vector3 const& pos2, float extra_radius, bool flip_coll_normal)
{
    float m;
    float distance;
    Urho3D::Vector3 nearest_point_at_line;
    nearestPointToLine(pos, pos1, pos2, &nearest_point_at_line, &m, &distance);
    if (m >= 0 && m <= 1) {
        if (distance < radius + extra_radius) {
            Urho3D::Vector3 coll_normal = (pos - nearest_point_at_line).Normalized();
            if (flip_coll_normal) {
                result.Push(Collision(-coll_normal, radius - distance));
            } else {
                result.Push(Collision(coll_normal, radius - distance));
            }
        }
    }
}

inline void getRawSphereCollisionToBox(Collisions& result, Urho3D::Vector3 const& size_half, Urho3D::Vector3 const& sphere_pos, float radius, float extra_radius, bool flip_coll_normal)
{
    unsigned result_original_size = result.Size();

    // First check hits inside box and in front of its faces
    // At X area
    if (sphere_pos.x_ >= -size_half.x_ && sphere_pos.x_ <= size_half.x_) {
        // At XY area
        if (sphere_pos.y_ >= -size_half.y_ && sphere_pos.y_ <= size_half.y_) {
            if (sphere_pos.z_ > 0) {
                float depth = size_half.z_ + radius - sphere_pos.z_;
                if (depth > -extra_radius) {
                    if (flip_coll_normal) {
                        result.Push(Collision(Urho3D::Vector3::BACK, depth));
                    } else {
                        result.Push(Collision(Urho3D::Vector3::FORWARD, depth));
                    }
                }
            } else {
                float depth = size_half.z_ + radius + sphere_pos.z_;
                if (depth > -extra_radius) {
                    if (flip_coll_normal) {
                        result.Push(Collision(Urho3D::Vector3::FORWARD, depth));
                    } else {
                        result.Push(Collision(Urho3D::Vector3::BACK, depth));
                    }
                }
            }
        }
        // At XZ area
        if (sphere_pos.z_ >= -size_half.z_ && sphere_pos.z_ <= size_half.z_) {
            if (sphere_pos.y_ > 0) {
                float depth = size_half.y_ + radius - sphere_pos.y_;
                if (depth > -extra_radius) {
                    if (flip_coll_normal) {
                        result.Push(Collision(Urho3D::Vector3::DOWN, depth));
                    } else {
                        result.Push(Collision(Urho3D::Vector3::UP, depth));
                    }
                }
            } else {
                float depth = size_half.y_ + radius + sphere_pos.y_;
                if (depth > -extra_radius) {
                    if (flip_coll_normal) {
                        result.Push(Collision(Urho3D::Vector3::UP, depth));
                    } else {
                        result.Push(Collision(Urho3D::Vector3::DOWN, depth));
                    }
                }
            }
        }
    }
    // At YZ area
    if (sphere_pos.y_ >= -size_half.y_ && sphere_pos.y_ <= size_half.y_ && sphere_pos.z_ >= -size_half.z_ && sphere_pos.z_ <= size_half.z_) {
        if (sphere_pos.x_ > 0) {
            float depth = size_half.x_ + radius - sphere_pos.x_;
            if (depth > -extra_radius) {
                if (flip_coll_normal) {
                    result.Push(Collision(Urho3D::Vector3::LEFT, depth));
                } else {
                    result.Push(Collision(Urho3D::Vector3::RIGHT, depth));
                }
            }
        } else {
            float depth = size_half.x_ + radius + sphere_pos.x_;
            if (depth > -extra_radius) {
                if (flip_coll_normal) {
                    result.Push(Collision(Urho3D::Vector3::RIGHT, depth));
                } else {
                    result.Push(Collision(Urho3D::Vector3::LEFT, depth));
                }
            }
        }
    }
    // If collision was found
    if (result.Size() > result_original_size) {
        dropAllExceptShallowestCollision(result, result_original_size);
        return;
    }

    // Check edges
    if (sphere_pos.x_ >= -size_half.x_ && sphere_pos.x_ <= size_half.x_) {
        if (sphere_pos.y_ > 0) {
            if (sphere_pos.z_ > 0) {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(size_half.x_, size_half.y_, size_half.z_), Urho3D::Vector3(-size_half.x_, size_half.y_, size_half.z_), extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(size_half.x_, size_half.y_, -size_half.z_), Urho3D::Vector3(-size_half.x_, size_half.y_, -size_half.z_), extra_radius, flip_coll_normal);
            }
        } else {
            if (sphere_pos.z_ > 0) {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(size_half.x_, -size_half.y_, size_half.z_), Urho3D::Vector3(-size_half.x_, -size_half.y_, size_half.z_), extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(size_half.x_, -size_half.y_, -size_half.z_), Urho3D::Vector3(-size_half.x_, -size_half.y_, -size_half.z_), extra_radius, flip_coll_normal);
            }
        }
    } else if (sphere_pos.y_ >= -size_half.y_ && sphere_pos.y_ <= size_half.y_) {
        if (sphere_pos.x_ > 0) {
            if (sphere_pos.z_ > 0) {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(size_half.x_, size_half.y_, size_half.z_), Urho3D::Vector3(size_half.x_, -size_half.y_, size_half.z_), extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(size_half.x_, size_half.y_, -size_half.z_), Urho3D::Vector3(size_half.x_, -size_half.y_, -size_half.z_), extra_radius, flip_coll_normal);
            }
        } else {
            if (sphere_pos.z_ > 0) {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(-size_half.x_, size_half.y_, size_half.z_), Urho3D::Vector3(-size_half.x_, -size_half.y_, size_half.z_), extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(-size_half.x_, size_half.y_, -size_half.z_), Urho3D::Vector3(-size_half.x_, -size_half.y_, -size_half.z_), extra_radius, flip_coll_normal);
            }
        }
    } else if (sphere_pos.z_ >= -size_half.z_ && sphere_pos.z_ <= size_half.z_) {
        if (sphere_pos.x_ > 0) {
            if (sphere_pos.y_ > 0) {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(size_half.x_, size_half.y_, size_half.z_), Urho3D::Vector3(size_half.x_, size_half.y_, -size_half.z_), extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(size_half.x_, -size_half.y_, size_half.z_), Urho3D::Vector3(size_half.x_, -size_half.y_, -size_half.z_), extra_radius, flip_coll_normal);
            }
        } else {
            if (sphere_pos.y_ > 0) {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(-size_half.x_, size_half.y_, size_half.z_), Urho3D::Vector3(-size_half.x_, size_half.y_, -size_half.z_), extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToLine(result, sphere_pos, radius, Urho3D::Vector3(-size_half.x_, -size_half.y_, size_half.z_), Urho3D::Vector3(-size_half.x_, -size_half.y_, -size_half.z_), extra_radius, flip_coll_normal);
            }
        }
    }
    if (result.Size() > result_original_size) {
        assert(result.Size() == result_original_size + 1);
        return;
    }

    // Check corners
    if (sphere_pos.x_ > 0) {
        if (sphere_pos.y_ > 0) {
            if (sphere_pos.z_ > 0) {
                getRawSphereCollisionToSphere(result, sphere_pos, Urho3D::Vector3(size_half.x_, size_half.y_, size_half.z_), radius, extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToSphere(result, sphere_pos, Urho3D::Vector3(size_half.x_, size_half.y_, -size_half.z_), radius, extra_radius, flip_coll_normal);
            }
        } else {
            if (sphere_pos.z_ > 0) {
                getRawSphereCollisionToSphere(result, sphere_pos, Urho3D::Vector3(size_half.x_, -size_half.y_, size_half.z_), radius, extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToSphere(result, sphere_pos, Urho3D::Vector3(size_half.x_, -size_half.y_, -size_half.z_), radius, extra_radius, flip_coll_normal);
            }
        }
    } else {
        if (sphere_pos.y_ > 0) {
            if (sphere_pos.z_ > 0) {
                getRawSphereCollisionToSphere(result, sphere_pos, Urho3D::Vector3(-size_half.x_, size_half.y_, size_half.z_), radius, extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToSphere(result, sphere_pos, Urho3D::Vector3(-size_half.x_, size_half.y_, -size_half.z_), radius, extra_radius, flip_coll_normal);
            }
        } else {
            if (sphere_pos.z_ > 0) {
                getRawSphereCollisionToSphere(result, sphere_pos, Urho3D::Vector3(-size_half.x_, -size_half.y_, size_half.z_), radius, extra_radius, flip_coll_normal);
            } else {
                getRawSphereCollisionToSphere(result, sphere_pos, Urho3D::Vector3(-size_half.x_, -size_half.y_, -size_half.z_), radius, extra_radius, flip_coll_normal);
            }
        }
    }
}

}

}

#endif
