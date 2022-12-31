// PigIron camera implementation.
//
// Copyright Evan Beeton 6/4/2005

#include "PI_Camera.h"

// "Zoom" the camera by translating along it's at-vector.
//
// In:		distance		Amount to zoom in/out.
void PI_Camera::Zoom(float distance)
{
	PI_Vec3 offset = target - pos;
	offset.Normalize();
	offset *= distance;
	pos += offset;
}

// Look at a specific point in space.
//
// In:		where			Where to look.
void PI_Camera::LookAt(const PI_Vec3 &where)
{
	target = where;

	// Update the camera's up vector.
	PI_Vec3 at = target - pos;
	at.Normalize();
	PI_Vec3 camX = at.Cross(PI_Vec3(0,1,0));
	camX.Normalize();
	up = camX.Cross(at);
}

bool PI_Camera::SphereInFrustum(const PI_Vec3 & where, float radius)
{
	for (unsigned char i = 0; i < 6; ++i)
		if (frustumPlanes[i].DotHomogenous(where) <= -radius)
			return false;
	return true;
}