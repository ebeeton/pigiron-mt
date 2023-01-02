// PigIron rendering interface.
//
// Copyright Evan Beeton 2/1/2005

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <memory>
using std::auto_ptr;
#include <vector>
using std::vector;
#include <map>
using std::map;
using std::pair;

#include "glext.h"
#include "PI_WorldTree.h"
#include "PI_Particle.h"
#include "PI_Camera.h"
#include "PI_DLight.h"
#include "PI_GUI.h"
#include "PI_Utils.h"

#pragma comment(lib, "Opengl32")
#pragma comment(lib, "Glu32")

// Descriptor for renderable static geometry.
class PI_RenderElement
{
	friend class PI_Render;
	PI_Mat44 worldXform;

	unsigned int displayList,		// The display list to render.
				    diffTexName,	// The diffuse texture to apply.
					normTexName; // The normal map texture to apply.
public:
	PI_RenderElement(const PI_Mat44 &mat, GLuint list, GLuint diffuseTexName, GLuint normalTexName)
		: worldXform(mat), displayList(list), diffTexName(diffuseTexName), normTexName(normalTexName)
	{ }

	PI_RenderElement(const PI_Mat44 &mat, const PI_MeshNode &node)
		: worldXform(mat), displayList(node.displayList), diffTexName(node.diffTexName), normTexName(node.normalTexName)
	{ }
};

// The main state-based rendering class.
class PI_Render
{
public:

	// Singleton accessor.
	static PI_Render &GetInstance(void) { return m_instance; }

	// Initialize the renderer.
	//
	// Returns					True if successful.
	bool Init(void);

// State management.
	enum RenderState
	{
		StartupState,
		LoadAssetState,			// Need to load assets.
		UpdateCameraState,		// Need to update the camera.
		ReadyState,				// Waiting for draw list additions.
		CommitFrameState,		// Draw list is locked, okay to render frame.
		UnloadAssetState,		// Need to unload assets.
		ShutdownState
	};

	// Commit all changes to the render list and render the current frame to the screen.
	void CommitFrame(void);

	// Tell the renderer to calculate and apply the camera during the next update.
	void SetUpdateCameraState(void);
	
	// Tell the renderer to unload all assets during the next update.
	void SetUnloadState(void);

	// Tell the renderer to shutdown during the next update.
	void SetShutdownState(void);

	// Update the renderer.
	//
	// Returns				False if the renderer has entered the shutdown state.
	bool PI_Render::Update(void);
	
	// Set the asset list to load.
	void SetAssetList(const vector<string> *assetList);

	// Accessor for loaded mesh hierarchies.
	//
	// In:		filename	What's it called?
	//
	// Out:		out			Where to put the handle.
	//
	// Returns				True if the mesh was found.
	bool GetMeshHandle(const char *filename, PI_Mesh &out) const;

	// Accessor for loaded textures.
	//
	// In:		filename	What's it called?
	//
	// Out:		texName		The generated texture name.
	//
	// Returns				True if the texture was found.
	bool GetTextureHandle(const char *filename, unsigned int &texName) const;

	// Find the first point of intersection between a ray and the world.
	//
	// In:		ray				The ray to test.
	//
	// Out:		intersectPoint	The point of intersection, if any.
	//
	// Returns					True if there was an intersection.
	bool FindFirstIntersectionWithWorld(const PI_Ray3 &ray, PI_Vec3 &intersectPoint) const;

	// Project the mouse coordinates parallel to the camera's at vector,
	// and find the point of intersection with the world geometry.
	bool ProjectMouseToWorldIntersection(PI_Vec3 &intersection) const;

	// Shut down the renderer.
	void Shutdown(void);

// Accessors / Modifiers

	// Accessor for the current render state.
	//
	// Returns:					The current render state.
	RenderState GetState(void) const;

	// Give the renderer a list of things to render.
	void SetRenderList(reusable_vector<PI_RenderElement> *vpList)
	{
		vpRenderList = vpList;
	}
	
	// Set the active camera.
	//
	// In:		cam				Pointer to the desired camera.
	//
	// Returns					The old camera.
	PI_Camera *SetActiveCam(PI_Camera *cam)
	{
		if (!cam)
			return cam;
		PI_Camera *old = pActiveCam;
		pActiveCam = cam;
		return old;
	}

	// Set the active directional light.
	PI_DLight *SetActiveDLight(PI_DLight *light)
	{
		if (!light)
			return light;

		PI_DLight *old = pActiveDLight;
		pActiveDLight = light;
		return old;
	}

	// Accessors for the viewport dimensions.
	int GetViewportWidth(void) const { return m_width; }
	int GetViewportHeight(void) const { return m_height; }

// Internal Routines
private:

	// Set initial OpenGL states.
	bool InitOpenGL(void);

	// Log some information about the current OpenGL connection.
	void LogGLConnection(void) const;

	// Log any OpenGL errors.
	//
	// In:		function		What function that called this one.
	void LogOpenGLErrors(const char *function) const;

	// Update the renderer's viewport.
	void UpdateViewport(void);

	// Apply the camera and build its frustum planes.
	void ApplyCamera(void);

	// Load a list of assets - can be meshes, textures and world geometry.
	//
	// Returns				True if ALL assets are loaded.
	bool LoadAssetList(void);

	// Load a PigIron Mesh (PIM) file.
	//
	// In:		filename		Name of desired file, can be relative or absolute.
	//
	// Out:		out				The loaded PIM data.
	//
	// Returns					True if the file was loaded successfully (or is already loaded)
	bool LoadPIM(const char *filename, PI_Mesh &out);

	// Load a PigIron Mesh (PIM) file as the world, and build the world tree.
	//
	// In:		filename		Name of desired file, can be relative or absolute.
	//
	// Returns					True if successful.
	bool LoadWorldPIM(const char *filename);

	// Load a 24- or 32-bit uncompressed TARGA image file.
	//
	// In:		filename		Name of desired file, can be relative or absolute.
	//			genMipMaps		Generate MIP maps?
	//
	// Out:		texName			Generated texture name.
	//
	// Returns					True if the file was loaded successfully (or is already loaded)
	friend bool PI_GUI::LoadFont(const char *name);
	bool LoadTarga(const char *filename, unsigned int &texName, bool genMipMaps);

	// Bind a specific texture to the rendering context.
	//
	// In:		texName			The texture to bind.
	friend void PI_GUI::RenderGUI(void) const;
	void BindTexture2D(unsigned int texName);

	// Render the scene and display it.
	void RenderScene(void);

	// Render all active shadow volumes.
	void RenderShadows(void);

	// Render a particle emitter.
	//
	// In:		emit			The emitter to render.
	void RenderParticleEmitter(const PI_ParticleEmitter *emit) const;

	// Render the entire world tree.
	// This function is recursive - pass the root node for the initial call.
	void RenderWorldTreeR(const PI_WorldTree::PI_WorldTreeNode *n) const;
	
	// Render some simple test geometry.
	void RenderTestGeometry(void) const;

	// Find the first point where a ray intersects the world geometry.
	// This function is recursive - pass the root node for the initial call.
	bool FindFirstIntersectionR(const PI_Ray3 &ray, PI_Vec3 &intersectPoint, const PI_WorldTree::PI_WorldTreeNode *n) const;

	// Release all assets from memory.
	void UnloadAllAssets(void);

	// Release all textures from memory.
	void UnloadAllTextures(void);

	// Release all static meshes from memory.
	void UnloadAllStaticMeshes(void);
	
// Data Members
private:

	RenderState state;

	// This class is a Singleton.
	static PI_Render m_instance;
	PI_Render(const PI_Render &rhs);
	PI_Render &operator=(const PI_Render &rhs);
	PI_Render(void) : m_HDC(0), m_HWND(0), m_HGLRC(0), m_width(0), m_height(0), pActiveCam(0), pActiveDLight(0), gui(PI_GUI::GetInstance()),
						activeTexStage0(0), activeTexStage1(0), pWorld(new PI_WorldTree),
						numTrisRendered(0), numNodesRendered(0), state(StartupState), vpAssetList(0), vpRenderList(0)
	{ }

	// Descriptor for memory-resident textures.
	struct PI_TexID
	{
		string filename;
		GLuint texName;
	};

	PI_Mat44 projectionMat;

	// Texture & geometry storage.
	vector <PI_TexID> vTextures;
	vector <PI_Mesh> vMeshes;

	// Everything to be rendered in the current frame.
	reusable_vector <PI_RenderElement> *vpRenderList;
	vector <const PI_ParticleEmitter *> vpEmitterList;

	// A map of edge data for all loaded meshes.
	// The key is the display list name that OpenGL generates for each unique mesh node.
	map<unsigned int, vector<PI_Edge> > edgeDataMap;

	// Required data members for binding OpenGL.
	HDC m_HDC;
	HWND m_HWND;
	HGLRC m_HGLRC;
	int m_width, m_height;

	// Keep track of the currently bound textures.
	mutable unsigned int activeTexStage0, activeTexStage1;

	// Scene lights and camera.
	PI_Camera *pActiveCam;
	PI_DLight *pActiveDLight;

	const vector<string> *vpAssetList;

	PI_GUI &gui;

	// Statistics for rendering.
	mutable unsigned int numTrisRendered, numNodesRendered;

	// The world.
	auto_ptr<PI_WorldTree> pWorld;

// Depricated Functions
private:

	// Add a mesh node to the render list.
	//
	// In:		mat				A world matrix where it should go.
	//			node			Desired node to render.
	//void AddToRenderList(const PI_Mat44 &mat, const PI_MeshNode &node);

	// Add a particle emitter to the render list.
	//void AddToRenderList(const PI_ParticleEmitter &emitter);
};