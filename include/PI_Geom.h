// PI_Geom.h - Geometry data support interfaces.
//
// Copyright Evan Beeton 5/2/2005

#pragma once

#include <string>
using std::string;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl\gl.h>


#include "PI_Math.h"

// A 3D edge shared by two triangles (plus their face normals).
// Useful for silhouette edge detection.
class PI_Edge
{
public:
	// Endpoints of the edge.
	PI_Vec3 verts[2];

	// Normals of the faces that border the edge.
	PI_Vec3 faceNormals[2];

	bool operator==(const PI_Edge &r) const
	{
		//return (verts[0] == r.verts[0] && verts[1] == r.verts[1]) || (verts[0] == r.verts[1] && verts[1] == r.verts[0]);
		return (faceNormals[0] == r.faceNormals[0] && faceNormals[1] == r.faceNormals[1]) ||
				(faceNormals[1] == r.faceNormals[0] && faceNormals[0] == r.faceNormals[1]);
	}
};

// A 3D quadrilateral.
class PI_Quad
{
	public:
		PI_Vec3 verts[4];
};

// A single 3D triangle with mapping coordinates and per-vertex normals.
class PI_Triangle
{
	// Descriptor for texture coordinates.
	struct TexCoord
	{
		float u, v;
		TexCoord(void) : u(0), v(0) {}
	};

public:
	PI_Triangle(void) : diffTex(0), normTex(0) { vertAlpha[0] = vertAlpha[1] = vertAlpha[2] = 1.0f; }
	
	PI_Vec3 GetCentroid(void) const
	{
		return PI_Vec3((verts[0].x + verts[1].x + verts[2].x) / 3,
					   (verts[0].y + verts[1].y + verts[2].y) / 3,
					   (verts[0].z + verts[1].z + verts[2].z) / 3);
	}
	
	PI_Vec3 verts[3],		// The verticies that compose the triangle.
			normals[3];		// Normals per vertex.
	TexCoord texCoord[3];	// Texture coordinates per vertex.
	float vertAlpha[3];		// Vertex alpha values.
	unsigned int diffTex,	// Diffuse texture name.
				 normTex;	// Normal map name.

	// Sort triangles by diffuse texture - this should help minimize
	// texture context switches when rendering arrays of triangles.
	bool operator<(const PI_Triangle &rhs) const
	{
		return diffTex < rhs.diffTex;
	}

	bool SharesEdge(const PI_Triangle &rhs, PI_Edge &sharedEdge) const;

	void GetFaceNormal(PI_Vec3 &normalOut) const;
};

enum GEOM_FLAGS { RENDERABLE = 0x01, TEXTURED = 0x02, CASTSHADOWS = 0x04, NORMALMAPPED = 0x08, WORLD = 0x10 };

// A single static geometric mesh object.
class PI_MeshNode
{
	// Only the renderer and meshes can copy nodes.
	friend class PI_Render;
	friend class PI_RenderElement;
	friend class PI_Mesh;
	PI_MeshNode &operator=(const PI_MeshNode &r);
	PI_MeshNode(const PI_MeshNode &r);

	PI_Mat44 ltm;					// LTM - local transform matrix.
	string name;					// The internal object name.
	PI_Vec3 aabb_min, aabb_max;		// Axis-aligned bounding box dimensions.
	unsigned int displayList,		// OpenGL display list.
					diffTexName,	// OpenGL texture name for diffuse.
					normalTexName;	// OpenGL texture name for normal mapping.
	float boundingRadius;			// Bounding sphere radius.
	unsigned char flags;			// Rendering flags.
	
public:

	PI_MeshNode(void) : displayList(0), diffTexName(0), normalTexName(0), flags(0), boundingRadius(0) { }

	const PI_Mat44 &GetLTM(void) const { return ltm; }
	const string &GetName(void) const { return name; }
	float GetBoundingRadius(void) const { return boundingRadius; }
	const PI_Vec3 &GetAABBMin(void) const { return aabb_min; }
	const PI_Vec3 &GetAABBMax(void) const { return aabb_max; }
};

// A group of static meshes.
class PI_Mesh
{
	// Only the renderer should be able to change data members.
	friend class PI_Render;
	
	// The PIM filename.
	string filename;

	// All the nodes in this group.
	PI_MeshNode *pNodes;

	// How many of them are there?
	unsigned int numNodes;

public:
	// Default constructor.
	PI_Mesh(void) : pNodes(0), numNodes(0)
	{ }

	// Destructor.
	~PI_Mesh();

	// Assignment operator.
	PI_Mesh &operator=(const PI_Mesh &r);

	// Copy constructor.
	PI_Mesh(const PI_Mesh &r);

	// Accessor for the filename.
	const string &GetFilename(void) const { return filename; }

	// Accessor for the number of nodes.
	int GetNumNodes(void) const { return numNodes; }
	
	// Accessors for the individual nodes.
	const PI_MeshNode &operator[](int index) const
	{
		return pNodes[index];
	}

	const PI_MeshNode &GetNode(int index) const
	{
		return pNodes[index];
	}
};