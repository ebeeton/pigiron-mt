// PigIron world geometry management.
//
// Copyright Evan Beeton 7/5/2005

#include <algorithm>
using std::sort;
#include <functional>
using std::greater;
#include <map>
using std::map;
using std::pair;

#include "PI_WorldTree.h"
#include "glext.h"

extern PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f;

PI_WorldTree::PI_WorldTreeNode::~PI_WorldTreeNode(void)
{
	const unsigned int Size = (unsigned int)vRenderData.size();
	for (unsigned int i = 0; i < Size; i++)
		glDeleteLists(vRenderData[i].displayList, 1);

	delete leftChild;
	delete rightChild;
}

// Functions for sorting a Node's triangles along the cardinal axes.
void SortNodeTrisX(PI_WorldTree::PI_WorldTreeNode &n)
{
	float midPoint = (n.max.x - n.min.x) * 0.5f + n.min.x;
	const unsigned int NumTris = (unsigned int)n.vTris.size();
	for (unsigned int t = 0; t < NumTris; ++t)
		if (n.vTris[t].GetCentroid().x < midPoint)
			n.leftChild->vTris.push_back(n.vTris[t]);
		else
			n.rightChild->vTris.push_back(n.vTris[t]);
}

void SortNodeTrisY(PI_WorldTree::PI_WorldTreeNode &n)
{
	float midPoint = (n.max.y - n.min.y) * 0.5f + n.min.y;
	const unsigned int NumTris = (unsigned int)n.vTris.size();
	for (unsigned int t = 0; t < NumTris; ++t)
		if (n.vTris[t].GetCentroid().y < midPoint)
			n.leftChild->vTris.push_back(n.vTris[t]);
		else
			n.rightChild->vTris.push_back(n.vTris[t]);
}

void SortNodeTrisZ(PI_WorldTree::PI_WorldTreeNode &n)
{
	float midPoint = (n.max.z - n.min.z) * 0.5f + n.min.z;
	const unsigned int NumTris = (unsigned int)n.vTris.size();
	for (unsigned int t = 0; t < NumTris; ++t)
		if (n.vTris[t].GetCentroid().z < midPoint)
			n.leftChild->vTris.push_back(n.vTris[t]);
		else
			n.rightChild->vTris.push_back(n.vTris[t]);
}

// Recursively process all nodes up to a depth of MAX_WORLDTREE_DEPTH
void PI_WorldTree::PI_WorldTreeNode::Process(unsigned short newDepth, unsigned int &nodeCount, unsigned int &leafNodeCount)
{
	unsigned int t, v;

	depth = newDepth;
	++nodeCount;

	// Build the AABB bounding volume.
	min = max = vTris[0].verts[0];
	const unsigned int NumTris = (unsigned int)vTris.size();
	for (t = 0; t < NumTris; t++)
	{
		for (v = 0; v < 3; v++)
		{
			if (max.x < vTris[t].verts[v].x)
				max.x = vTris[t].verts[v].x;
			if (max.y < vTris[t].verts[v].y)
				max.y = vTris[t].verts[v].y;
			if (max.z < vTris[t].verts[v].z)
				max.z = vTris[t].verts[v].z;

			if (min.x > vTris[t].verts[v].x)
				min.x = vTris[t].verts[v].x;
			if (min.y > vTris[t].verts[v].y)
				min.y = vTris[t].verts[v].y;
			if (min.z > vTris[t].verts[v].z)
				min.z = vTris[t].verts[v].z;
		}
	}

	// Compute the center point.
	center.x = (max.x - min.x) * 0.5f + min.x;
	center.y = (max.y - min.y) * 0.5f + min.y;
	center.z = (max.z - min.z) * 0.5f + min.z;

	static PI_Vec3 aabb_normals[6] =
	{
		PI_Vec3(-1,0,0),	// Left
		PI_Vec3(1,0,0),		// Right
		PI_Vec3(0,-1,0),	// Bottom
		PI_Vec3(0,1,0),		// Tops
		PI_Vec3(0,0,1),		// Near
		PI_Vec3(0,0,1),		// Far
	};

	// Ax + By + Cz + D = 0, where D is -Normal dot PointOnPlane
	aabb_planes[PlaneLeft].Set(aabb_normals[PlaneLeft], (-aabb_normals[PlaneLeft]).Dot(min));
	aabb_planes[PlaneRight].Set(aabb_normals[PlaneRight], (-aabb_normals[PlaneRight]).Dot(max));
	aabb_planes[PlaneBottom].Set(aabb_normals[PlaneBottom], (-aabb_normals[PlaneBottom]).Dot(min));
	aabb_planes[PlaneTop].Set(aabb_normals[PlaneTop], (-aabb_normals[PlaneTop]).Dot(max));
	aabb_planes[PlaneNear].Set(aabb_normals[PlaneNear], (-aabb_normals[PlaneNear]).Dot(max));
	aabb_planes[PlaneFar].Set(aabb_normals[PlaneFar], (-aabb_normals[PlaneFar]).Dot(min));

	// Determine if this node should become a leaf node.
	if (depth >= (MAX_WORLDTREE_DEPTH - 1) || vTris.size() <= LEAF_POLY_THRESHOLD)
	{
		// Build a display list for each group of triangles with the same textures.
		sort(vTris.begin(), vTris.end());
		unsigned int curDiffTex = vTris[0].diffTex, curNormTex = vTris[0].normTex;
		vector<PI_Triangle> temp;
		for (unsigned int t = 0; t < NumTris; ++t)
		{
			// Is it time to generate a new list?
			if (curDiffTex != vTris[t].diffTex || curNormTex != vTris[t].normTex || t == vTris.size() - 1)
			{
				if (t == NumTris - 1)
					// This is the last group, so include the current triangle.
					temp.push_back(vTris[t]);

				// Build a display list for what we currently have.
				RenderData rd(0, curDiffTex, curNormTex);
				rd.displayList = glGenLists(1);
				
				glNewList(rd.displayList, GL_COMPILE);
				glBegin(GL_TRIANGLES);
				const unsigned int TrisInGroup = (unsigned int)temp.size();
				for (unsigned int i = 0; i < TrisInGroup; ++i)
				{
					glColor4f(1,1,1,temp[i].vertAlpha[0]);
					glNormal3fv(temp[i].normals[0]);
					glMultiTexCoord2f(GL_TEXTURE0, temp[i].texCoord[0].u, temp[i].texCoord[0].v);
					//glMultiTexCoord2f(GL_TEXTURE1, temp[i].texCoord[0].u, temp[i].texCoord[0].v);
					glVertex3fv(temp[i].verts[0]);
			
					glColor4f(1,1,1,temp[i].vertAlpha[1]);
					glNormal3fv(temp[i].normals[1]);
					glMultiTexCoord2f(GL_TEXTURE0, temp[i].texCoord[1].u, temp[i].texCoord[1].v);
					//glMultiTexCoord2f(GL_TEXTURE1, temp[i].texCoord[1].u, temp[i].texCoord[1].v);
					glVertex3fv(temp[i].verts[1]);

					glColor4f(1,1,1,temp[i].vertAlpha[2]);
					glNormal3fv(temp[i].normals[2]);
					glMultiTexCoord2f(GL_TEXTURE0, temp[i].texCoord[2].u, temp[i].texCoord[2].v);
					//glMultiTexCoord2f(GL_TEXTURE1, temp[i].texCoord[2].u, temp[i].texCoord[2].v);
					glVertex3fv(temp[i].verts[2]);
				}
				glEnd();
				glEndList();
				vRenderData.push_back(rd);

				// Prepare for the next group, if there is one.
				// If there is no next group, the loop is about to end.
				temp.clear();
				temp.push_back(vTris[t]);
				curDiffTex = vTris[t].diffTex;
				curNormTex = vTris[t].normTex;

			}
			else
				// This triangle belongs to the current group.
				temp.push_back(vTris[t]);
		}

		// This is a leaf node, so there's nothing else to do.
		++leafNodeCount;
		return;
	}

	// We can go at least one more level deep, so allocate child nodes.
	leftChild = new PI_WorldTreeNode;
	rightChild = new PI_WorldTreeNode;

	// Split the node's triangles amongst its children, based on where they lie on the
	// longest axis of the node's bounding volume.
	typedef void (*SortFunc)(PI_WorldTreeNode &);
	map<float, SortFunc, greater<float> > sortMap;
	sortMap.insert(pair<float, SortFunc>(max.x - min.x, SortNodeTrisX));
	sortMap.insert(pair<float, SortFunc>(max.y - min.y, SortNodeTrisY));
	sortMap.insert(pair<float, SortFunc>(max.z - min.z, SortNodeTrisZ));
	sortMap.begin()->second(*this);

	// Make sure the children actually received some geometry before processing them.
	if (leftChild->vTris.size())
		leftChild->Process(depth + 1, nodeCount, leafNodeCount);
	else
	{
		delete leftChild;
		leftChild = 0;
	}

	if (rightChild->vTris.size())
		rightChild->Process(depth + 1, nodeCount, leafNodeCount);
	else
	{
		delete rightChild;
		rightChild = 0;
	}

	// Actual geometry should only be stored in leaf nodes.
	vTris.clear();
}

PI_WorldTree::~PI_WorldTree(void)
{
	Clear();
}

bool PI_WorldTree::AddToWorld(const PI_Triangle *pTris, unsigned int num)
{
	// Expand the world triangle buffer and copy the new triangles in.
	if (!(pWorldTris = (PI_Triangle *)realloc(pWorldTris, sizeof(PI_Triangle) * (numWorldTris + num))))
		return false;
    memcpy(pWorldTris + numWorldTris, pTris, sizeof(PI_Triangle) * num);
	numWorldTris += num;
	return true;
}

void PI_WorldTree::BuildWorldTree(void)
{
	// Make sure the old hierarchy is gone.
	delete pRootNode;

	// Create the root node and add all the world geometry to it.
	pRootNode = new PI_WorldTreeNode;
	pRootNode->vTris.insert(pRootNode->vTris.begin(), pWorldTris, pWorldTris + numWorldTris);

	// Build the tree recursively.
	pRootNode->Process(0, nodeCount, leafNodeCount);

	// Once the world geometry is in the tree, there's no need to store it.
	free(pWorldTris);
	pWorldTris = 0;
}

void PI_WorldTree::Clear(void)
{
	delete pRootNode;
	pRootNode = 0;
	nodeCount = 0;

	free(pWorldTris);
	pWorldTris = 0;
	numWorldTris = 0;
}
