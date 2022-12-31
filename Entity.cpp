// Implementation of the base Entity class.
//
// Copyright Evan Beeton 7/12/2005

#include "Entity.h"
#include "PI_Render.h"
#include "PI_Utils.h"

// Rendering interface instance.
PI_Render &Entity::renderer = PI_Render::GetInstance();

// Orient the entity to the world geometry underneath it.
//
// In:		aabb_min	Axis-aligned bounding box minimum extents.
//			aabb_max	Axis-aligned bounding box maximum extents.
void Entity::AlignToWorld(const PI_Vec3 &aabb_min, const PI_Vec3 &aabb_max)
{
	PI_Vec3 down(0,-1,0), front = aabb_max, right = aabb_min, left = right;

	// Calculate the offset vectors based on the entity's AABB.
	front.x = front.y = 0;
	right.y = left.y = 0;
	left.x = -right.x;
	front.Rotate(worldMat);
	left.Rotate(worldMat);
	right.Rotate(worldMat);

	// The points used to create a triangle around the entity.
	PI_Vec3 collVec2(worldMat.GetTranslation() + front),				// Front
			collVec1(worldMat.GetTranslation() + left),	// Back left
			collVec0(worldMat.GetTranslation() + right);	// Back right

	// Create the rays used to surround the entity and project down into the world.
	PI_Ray3 collRay0(collVec0, down),
			collRay1(collVec1, down),
			collRay2(collVec2, down);

	// Check for world collision.
	renderer.FindFirstIntersectionWithWorld(collRay0, collVec0);
	renderer.FindFirstIntersectionWithWorld(collRay1, collVec1);
	renderer.FindFirstIntersectionWithWorld(collRay2, collVec2);

	// Rebuild the entity's world matrix.
	PI_Vec3 newX = collVec1 - collVec0;
	newX.Normalize();
	PI_Vec3 tempZ = collVec2 - collVec1;
	tempZ.Normalize();
	worldMat.SetAxisY(tempZ.Cross(newX));
	worldMat.SetAxisZ(newX.Cross(worldMat.GetAxisY()));
	worldMat.SetAxisX(newX);

	// Set the world matrix Y component to the average of the world collision points.	
	worldMat.mat[13] = (collVec0.y + collVec1.y + collVec2.y) / 3.0f;
}
