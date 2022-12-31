// PigIron camera interface.
//
// Copyright Evan Beeton 6/4/2005

#pragma once

#include "PI_Math.h"

class PI_Camera
{
	friend class PI_Render;

	PI_Plane3 frustumPlanes[6];

	PI_Vec3 pos,	// Where's it at?
			target,	// What specific point is it looking at?
			up;		// Which way's up?

public:

	// Default constructor.
	PI_Camera(void) : up(0,1,0), target(0,0,-1)
	{ }

	// Powerful constructor.
	PI_Camera(const PI_Vec3 &_pos, const PI_Vec3 &_target, const PI_Vec3 &_up) : pos(_pos), target(_target), up(_up)
	{ }

	// Accessor for the position.
	const PI_Vec3 &GetPosition(void) const { return pos; }
	
	// Accessor for the camera's target.
	const PI_Vec3 &GetTarget(void) const { return target; }

	// Move the camera to a specific spot.
	//
	// In:		where			Where to put it.
	void MoveTo(const PI_Vec3 &where) { pos = where; }
	
	// Offset the camera.
	//
	// In:		offset			How to translate the camera.
	void Translate(const PI_Vec3 &offset) { pos += offset; }

	// "Zoom" the camera by translating along it's at-vector.
	//
	// In:		distance		Amount to zoom in/out.
	void Zoom(float distance);

	// Look at a specific point in space.
	//
	// In:		where			Where to look.
	void LookAt(const PI_Vec3 &where);

	bool SphereInFrustum(const PI_Vec3 & where, float radius);
};