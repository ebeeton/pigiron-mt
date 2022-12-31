// Interface for the base Entity class.
//
// Copyright Evan Beeton 6/4/2005

#pragma once

#include "PI_Render.h"
#include "PI_Utils.h"

class Entity
{
protected:

	// Rendering interface instance.
	static PI_Render &renderer;

	// The entity's world transform.
	PI_Mat44 worldMat;

public:

	Entity(void) { }

	virtual ~Entity(void) { }

	// Initialize the entity.
	//
	// Returns				True if successful.
	virtual bool Init(void) = 0;

	// Update the entity.
	//
	// In:		deltaTime	How much time has elapsed since the last update. (milliseconds)
	virtual void Update(float deltaTime) = 0;

	// Add the entities' mesh data to the render list.
	//
	// In:		vpList		The list to add to.
	virtual void AddToRenderList(reusable_vector<PI_RenderElement> &vpList) = 0;

	// Accessor for the bounding radius.
	//
	// Returns				The bounding radius.
	virtual float GetBoundingRadius(void) const { return 0; }

	// Orient the entity to the world geometry underneath it.
	//
	// In:		aabb_min	Axis-aligned bounding box minimum extents.
	//			aabb_max	Axis-aligned bounding box maximum extents.
	void AlignToWorld(const PI_Vec3 &aabb_min, const PI_Vec3 &aabb_max);

	// Accessors/Modifiers.
	const PI_Mat44 &GetWorldMat(void) const { return worldMat; }
	void SetWorldMat(const PI_Mat44 &w) { worldMat = w; }
	const PI_Vec3 GetWorldTranslation(void) const { return worldMat.GetTranslation(); }
};