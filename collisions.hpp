#ifndef URHOEXTRAS_COLLISIONS_HPP
#define URHOEXTRAS_COLLISIONS_HPP

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Vector3.h>

#include <cassert>

namespace UrhoExtras
{

struct Collision
{
	Urho3D::Vector3 normal;
	float depth;

	inline Collision() {}
	inline Collision(Urho3D::Vector3 const& normal, float depth) :
	normal(normal),
	depth(depth)
	{
		assert(fabs(normal.Length() - 1.0) < 0.01);
	}
};
typedef Urho3D::PODVector<Collision> Collisions;

class CollisionShape
{

public:

	enum Type
	{
		SPHERE,
		CAPSULE
	};

	inline static CollisionShape createSphere(Urho3D::Vector3 const& pos, float radius)
	{
		CollisionShape cs;
		cs.type = SPHERE;
		cs.pos1 = pos;
		cs.radius = radius;
		return cs;
	}

	inline static CollisionShape createCapsule(Urho3D::Vector3 const& pos1, Urho3D::Vector3 const& pos2, float radius)
	{
		CollisionShape cs;
		cs.type = CAPSULE;
		cs.pos1 = pos1;
		cs.pos2 = pos2;
		cs.radius = radius;
		return cs;
	}

	inline Type getType() const { return type; }
	inline Urho3D::Vector3 getPosition() const { return pos1; }
	inline Urho3D::Vector3 getSecondPosition() const { return pos2; }
	inline float getRadius() const { return radius; }

	inline Urho3D::BoundingBox getBoundingBox() const
	{
		Urho3D::Vector3 radius3(radius, radius, radius);
		Urho3D::BoundingBox bb(pos1 + radius3, pos1 - radius3);
		bb.Merge(pos2 + radius3);
		bb.Merge(pos2 - radius3);
		return bb;
	}

	inline void move(Urho3D::Vector3 const& v)
	{
		pos1 += v;
		if (type == CAPSULE) {
			pos2 += v;
		}
	}

	void getCollisionsToTriangle(Collisions& result, Urho3D::Vector3 const& pos1, Urho3D::Vector3 const& pos2, Urho3D::Vector3 const& pos3, Urho3D::BoundingBox const& bb, float extra_radius = -1, bool only_front_collisions = false) const;

private:

	Type type;

	Urho3D::Vector3 pos1, pos2;
	float radius;

	inline CollisionShape() : type(SPHERE) {}
};

// Calculates position delta to get object out of walls. Note,
// that this function invalidates depth values of collisions
// and removes those collisions that does not really hit walls.
Urho3D::Vector3 moveOutFromCollisions(Collisions& colls);

}

#endif
