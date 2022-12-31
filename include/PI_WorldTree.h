// PigIron world geometry management. 
//
// Copyright Evan Beeton 7/5/2005

#pragma once

#include <vector>
using std::vector;

#include "PI_Geom.h"

// Leaf nodes are created when the tree depth reaches MAX_WORLDTREE_DEPTH,
// or the number of polygons in a node is less than or equal to LEAF_POLY_THRESHOLD,
// whichever comes first.
#define MAX_WORLDTREE_DEPTH 12
#define LEAF_POLY_THRESHOLD 20

class PI_WorldTree
{
		struct PI_WorldTreeNode
		{
			
			struct RenderData
			{
				unsigned int displayList,	// OpenGL display list.
							 diffTexName,	// OpenGL texture name for diffuse.
							 normTexName;	// OpenGL texture name for normal mapping.
				RenderData(unsigned int _displayList = 0, unsigned int _diffTexName = 0, unsigned int _normTexName = 0)
					: displayList(_displayList), diffTexName(_diffTexName), normTexName(_normTexName) { }
			};

			PI_Plane3 aabb_planes[6];
			
			vector<RenderData> vRenderData;

			// All the triangles in the node.
			vector<PI_Triangle> vTris;
	
			PI_Vec3 min, max, center;
		
			PI_WorldTreeNode *leftChild, *rightChild;

			// How far down is the node in the world tree?
			unsigned short depth;

			PI_WorldTreeNode(void) : leftChild(0), rightChild(0), depth(0) {}
			~PI_WorldTreeNode(void);

			// Recursively process all nodes up to a depth of MAX_WORLDTREE_DEPTH
			void Process(unsigned short newDepth, unsigned int &nodeCount, unsigned int &leafNodeCount);
		};

		// Functions for sorting a Node's triangles along the cardinal axes.
		friend void SortNodeTrisX(PI_WorldTreeNode &n);
		friend void SortNodeTrisY(PI_WorldTreeNode &n);
		friend void SortNodeTrisZ(PI_WorldTreeNode &n);

		friend class PI_Render;
		PI_WorldTree(void)
			: pRootNode(0), pWorldTris(0), numWorldTris(0), nodeCount(0), leafNodeCount(0)
		{ }
		PI_WorldTree &operator=(const PI_WorldTree &r);
		PI_WorldTree(const PI_WorldTree &r);

		PI_WorldTreeNode *pRootNode;

		PI_Triangle *pWorldTris;
		
		unsigned int numWorldTris, nodeCount, leafNodeCount;

	public:

		~PI_WorldTree(void);
		
		bool AddToWorld(const PI_Triangle *pTris, unsigned int num);

		void BuildWorldTree(void);

		void Clear(void);
};