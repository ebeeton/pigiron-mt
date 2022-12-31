// PigIron directional light implementation.
//
// Copyright Evan Beeton 9/24/2005

#include "PI_DLight.h"

const float PI_DLight::ShadowLength = 20.0f;

void PI_DLight::SetDirection(const PI_Vec3 &direction)
{
	dir = direction;
	dir.Normalize();
}

void PI_DLight::SetDirection(float x, float y, float z)
{
	dir.x = x, dir.y = y, dir.z = z;
	dir.Normalize();
}

void PI_DLight::BuildShadowVolume(const vector<PI_Edge> &vEdges, const PI_Mat44 &worldXform)
{
	// Look for all silhouette edges.
	PI_Quad qTemp;

	PI_Vec3 lightDirTR = dir;
	lightDirTR.TransposedRotate(worldXform);
	const unsigned int NumEdges = (unsigned int)vEdges.size();
	for (unsigned int i = 0; i < NumEdges; i++)
	{
		if (lightDirTR.Dot(vEdges[i].faceNormals[0]) > 0 && lightDirTR.Dot(vEdges[i].faceNormals[1]) < 0)
		{
			// Face 0 is towards the light and Face 1 is away
			qTemp.verts[0] = worldXform * vEdges[i].verts[1];
			qTemp.verts[1] = worldXform * vEdges[i].verts[0];
			qTemp.verts[2] = qTemp.verts[1] - dir * ShadowLength;
			qTemp.verts[3] = qTemp.verts[0] - dir * ShadowLength;

			// Add the quad to the light's shadow volume.
			if (shadowQuadCount < vShadowVolume.size())
				vShadowVolume[shadowQuadCount++] = qTemp;
			else
			{
				vShadowVolume.push_back(qTemp);
				++shadowQuadCount;
			}
		}
		else if (lightDirTR.Dot(vEdges[i].faceNormals[0]) < 0 && lightDirTR.Dot(vEdges[i].faceNormals[1]) > 0)
		{
			// Face 0 is away from the light and Face 1 is towards
			qTemp.verts[0] = worldXform * vEdges[i].verts[0];
			qTemp.verts[1] = worldXform * vEdges[i].verts[1];
			qTemp.verts[2] = qTemp.verts[1] - dir * ShadowLength;
			qTemp.verts[3] = qTemp.verts[0] - dir * ShadowLength;

			// Add the quad to the light's shadow volume.
			if (shadowQuadCount < vShadowVolume.size())
				vShadowVolume[shadowQuadCount++] = qTemp;
			else
			{
				vShadowVolume.push_back(qTemp);
				++shadowQuadCount;
			}
		}
	}
}