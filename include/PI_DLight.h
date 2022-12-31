// PigIron directional light interface.
//
// Copyright Evan Beeton 9/24/2005

#pragma once

#include <vector>
using std::vector;

#include "PI_Geom.h"

class PI_DLight
{
	friend class PI_Render;
	vector<PI_Quad> vShadowVolume;
	PI_Vec3 dir;
	unsigned int shadowQuadCount;

	// How far are shadow quads extruded?
	static const float ShadowLength;

public:

	PI_DLight(const PI_Vec3 &direction) : dir(direction), shadowQuadCount(0) { }
	PI_DLight(float x = 0, float y = 0, float z = 0) : dir(x, y, z), shadowQuadCount(0) { }

	const PI_Vec3 &GetDirection(void) const
	{
		return dir;
	}

	void SetDirection(const PI_Vec3 &direction);

	void SetDirection(float x, float y, float z);

	void BuildShadowVolume(const vector<PI_Edge> &vEdges, const PI_Mat44 &worldXform);

	void ClearShadowVolume(void) { shadowQuadCount = 0; }
};