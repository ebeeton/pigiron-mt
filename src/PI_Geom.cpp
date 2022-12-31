// PI_Geom.cpp - Geometry data support implementations.
//
// Copyright Evan Beeton 6/11/2005

#include "PI_Geom.h"

bool PI_Triangle::SharesEdge(const PI_Triangle &rhs, PI_Edge &sharedEdge) const
{
	const PI_Vec3 *v1 = 0, *v2 = 0;
	
	// Look for a shared edge among all the verts.
	for (int i = 0; i < 3; ++i)
		for (int j = 2; j >= 0; --j)
		{
			// Did we find a shared vertex?
			if (verts[i] == rhs.verts[j])
				if (!v1)
					// This was the first one found.
					v1 = &verts[i];
				else
					// There can be only two (unless the mesh is screwed up), so this must be the other.
					v2 = &verts[i];
		
			if (v1 && v2)
			{
				// Found two shared verts - fill out the edge descriptor with the shared verts
				// and face normals from the two triangles.
				sharedEdge.verts[0] = *v1;
				sharedEdge.verts[1] = *v2;

				GetFaceNormal(sharedEdge.faceNormals[0]);
				rhs.GetFaceNormal(sharedEdge.faceNormals[1]);

				return true;
			}
		}

	// No shared edge found.
	return false;
}

void PI_Triangle::GetFaceNormal(PI_Vec3 &normalOut) const
{
	normalOut = (verts[1] - verts[0]).Cross(verts[2] - verts[0]);
	normalOut.Normalize();
}

// The assignment operator is a private data member of PI_MeshNode.
// PI_Render is the only friend allowed to make copies.
PI_MeshNode &PI_MeshNode::operator=(const PI_MeshNode &r)
{
	if (this != &r)
	{
		ltm = r.ltm;
		name = r.name;
		aabb_min = r.aabb_min;
		aabb_max = r.aabb_max;
		displayList = r.displayList;
		diffTexName = r.diffTexName;
		normalTexName = r.normalTexName;
		flags = r.flags;
		boundingRadius = r.boundingRadius;
	}
	return *this;
}

// The assignment operator is a private data member of PI_MeshNode.
// PI_Render is the only friend allowed to make copies.
PI_MeshNode::PI_MeshNode(const PI_MeshNode &r)
{
	ltm = r.ltm;
	name = r.name;
	aabb_min = r.aabb_min;
	aabb_max = r.aabb_max;
	displayList = r.displayList;
	diffTexName = r.diffTexName;
	normalTexName = r.normalTexName;
	flags = r.flags;
	boundingRadius = r.boundingRadius;
}

PI_Mesh::~PI_Mesh()
{
	delete [] pNodes;
}

PI_Mesh &PI_Mesh::operator=(const PI_Mesh &r)
{
	if (this != &r)
	{
		filename = r.filename;
		numNodes = r.numNodes;
		delete [] pNodes;
		pNodes = new PI_MeshNode[numNodes];
		for (unsigned int i = 0; i < numNodes; i++)
			pNodes[i] = r.pNodes[i];
		
	}
	return *this;
}

// Copy constructor.
PI_Mesh::PI_Mesh(const PI_Mesh &r)
{
	filename = r.filename;
	numNodes = r.numNodes;
	pNodes = new PI_MeshNode[numNodes];
		for (unsigned int i = 0; i < numNodes; i++)
			pNodes[i] = r.pNodes[i];
}