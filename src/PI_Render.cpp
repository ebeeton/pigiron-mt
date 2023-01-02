// PigIron rendering implementation.
//
// Copyright Evan Beeton 2/1/2005

#include <fstream>
using std::ifstream;
using std::ios;
using std::ios_base;

#include <cmath>

#include "PI_Render.h"
#include "PI_Logger.h"
#include "PI_Utils.h"

#define RGBA_WHITE 1.0f, 1.0f, 1.0f, 1.0f
#define RGBA_RED 1.0f, 0, 0, 1.0f
#define RGB_RED 1.0f, 0, 0
#define RGBA_GREEN 0, 1.0f, 0, 1.0f
#define RGB_GREEN 0, 1.0f, 0
#define RGBA_BLUE 0, 0, 1.0f, 1.0f
#define RGB_BLUE 0, 0, 1.0f

PI_Render PI_Render::m_instance;
PFNGLACTIVETEXTUREPROC glActiveTextureARB;
PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f;
extern CRITICAL_SECTION g_cs;

// BEGIN PUBLIC MEMBER FUNCTIONS

// Initialize the renderer.
//
// Returns					True if successful.
bool PI_Render::Init(void)
{
	EnterCriticalSection(&g_cs);

	extern HWND hWnd;
	if (!(m_HWND = hWnd))
	{
		ERRORBOX("Invalid HWND passed to PI_Render::Init");
		LeaveCriticalSection(&g_cs);
		return false;
	}

	// What kind of drawing format are we looking for?
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,					// Color buffer bit depth
		0, 0, 0, 0, 0, 0,
		0,					// Alpha buffer bit depth
		0,
		0,					// Accumulation buffer bit depth
		0, 0, 0, 0,
		16,					// Depth buffer bit depth
		1,					// Stencil buffer bit depth
	};

	// Can we get a device context?
	if (!(m_HDC = GetDC(m_HWND)))
	{
		ERRORBOX("GetDC failed");
		Shutdown();
		LeaveCriticalSection(&g_cs);
		return false;
	}

	// Can the device context support the pixel format we want?
	int pixelFormatIndex = 0;
	if (!(pixelFormatIndex = ChoosePixelFormat(m_HDC, &pfd)))
	{
		ERRORBOX("ChoosePixelFormat failed");
		Shutdown();
		LeaveCriticalSection(&g_cs);
		return false;
	}
	if (!SetPixelFormat(m_HDC, pixelFormatIndex, &pfd))
	{
		ERRORBOX("SetPixelFormat failed");
		Shutdown();
		LeaveCriticalSection(&g_cs);
		return false;
	}

	// Attempt to create and activate a rendering context.
	if (!(m_HGLRC = wglCreateContext(m_HDC)))
	{
		ERRORBOX("wglCreateContext failed");
		Shutdown();
		LeaveCriticalSection(&g_cs);
		return false;
	}
	if (!wglMakeCurrent(m_HDC, m_HGLRC))
	{
		ERRORBOX("wglMakeCurrent failed");
		Shutdown();
		LeaveCriticalSection(&g_cs);
		return false;
	}
	
	// Initialize OpenGL states.
	if (!InitOpenGL()) {
		LeaveCriticalSection(&g_cs);
		return false;
	}
	LogGLConnection();

	// Ready to rock!
	state = ReadyState;
	LeaveCriticalSection(&g_cs);
	return true;
}

// Commit all changes to the render list and render the current frame to the screen.
void PI_Render::CommitFrame(void)
{
	EnterCriticalSection(&g_cs);
	state = CommitFrameState;
	LeaveCriticalSection(&g_cs);
}

// Tell the renderer to calculate and apply the camera during the next update.
void PI_Render::SetUpdateCameraState(void)
{
	EnterCriticalSection(&g_cs);
	state = UpdateCameraState;
	LeaveCriticalSection(&g_cs);
}

// Tell the renderer to unload all assets during the next update.
void PI_Render::SetUnloadState(void)
{
	EnterCriticalSection(&g_cs);
	state = UnloadAssetState;
	LeaveCriticalSection(&g_cs);
}

// Tell the renderer to shutdown during the next update.
void PI_Render::SetShutdownState(void)
{
	EnterCriticalSection(&g_cs);
	state = ShutdownState;
	LeaveCriticalSection(&g_cs);
}

// Update the renderer.
//
// Returns				False if the renderer has entered the shutdown state.
bool PI_Render::Update(void)
{
	//EnterCriticalSection(&cs);
	switch (state)
	{
	case LoadAssetState:
		if (!LoadAssetList())
			ERRORBOX("PI_Render::LoadAssetList() failed!\n");
		break;
	case UpdateCameraState:
		ApplyCamera();
		break;
	case CommitFrameState:
		RenderScene();
		break;
	case UnloadAssetState:
		UnloadAllAssets();
		break;
	case ShutdownState:
		return false;
	}	
	//LeaveCriticalSection(&cs);
	return true;
}

// Set the asset list to load.
void PI_Render::SetAssetList(const vector<string> *assetList)
{
	EnterCriticalSection(&g_cs);
	vpAssetList = assetList;
	state = LoadAssetState;
	LeaveCriticalSection(&g_cs);
}

// Accessor for loaded mesh hierarchies.
//
// In:		filename	What's it called?
//
// Out:		out			Where to put the handle.
//
// Returns				True if the mesh was found.
bool PI_Render::GetMeshHandle(const char *filename, PI_Mesh &out) const
{
	const unsigned int NumMeshes = (unsigned int)vMeshes.size();
	for (unsigned int i = 0; i < NumMeshes; i++)
		if (!strcmp(filename, vMeshes[i].filename.c_str()))
		{
			// Found it!
			out = vMeshes[i];
			return true;
		}

	// Mesh not found!
	return false;
}

// Accessor for loaded textures.
//
// In:		filename	What's it called?
//
// Out:		texName		The generated texture name.
//
// Returns				True if the texture was found.
bool PI_Render::GetTextureHandle(const char *filename, unsigned int &texName) const
{
	// Don't reload a texture that's already resident.
	const unsigned int NumTextures = (unsigned int)vTextures.size();
	for (unsigned int i = 0; i < NumTextures; i++)
		if (!strcmp(vTextures[i].filename.c_str(), filename))
		{
			texName = vTextures[i].texName;
			return true;
		}

		// Texture not found!
		return false;
}

// Find the first point of intersection between a ray and the world.
//
// In:		ray				The ray to test.
//
// Out:		intersectPoint	The point of intersection, if any.
//
// Returns					True if there was an intersection.
bool PI_Render::FindFirstIntersectionWithWorld(const PI_Ray3 &ray, PI_Vec3 &intersectPoint) const
{
	if (!pWorld->pRootNode)
		return false;
	return FindFirstIntersectionR(ray, intersectPoint, pWorld->pRootNode);
}

// Project the mouse coordinates parallel to the camera's at vector,
// and find the point of intersection with the world geometry.
bool PI_Render::ProjectMouseToWorldIntersection(PI_Vec3 &intersection) const
{
	double modelview[16], projection[16], worldx, worldy, worldz;
	int viewport[4];
	POINT mouse;

	GetCursorPos(&mouse);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Project the mouse to the near clip plane.
	if (GL_FALSE == gluUnProject(mouse.x, m_height - mouse.y, 0, modelview, projection, viewport, &worldx, &worldy, &worldz))
		return false;
	PI_Vec3 nearMouse((float)worldx, (float)worldy, (float)worldz);
	
	// Project the mouse to the far clip plane.
	if (GL_FALSE == gluUnProject(mouse.x, m_height - mouse.y, 1.0, modelview, projection, viewport, &worldx, &worldy, &worldz))
		return false;
	PI_Vec3 farMouse((float)worldx, (float)worldy, (float)worldz);

	PI_Ray3 mouseRay(pActiveCam->pos, farMouse - nearMouse);
	//mouseRay.dir.Normalize();

	return FindFirstIntersectionR(mouseRay, intersection, pWorld->pRootNode);
}

// Shut down the renderer.
void PI_Render::Shutdown(void)
{	
	EnterCriticalSection(&g_cs);

	// Resume default state for cameras and lights.
	pActiveCam = 0;
	pActiveDLight = 0;

	// Unload everything.
	UnloadAllAssets();

	// Unbind and free the rendering context.
	if (m_HGLRC)
	{
		if (!wglMakeCurrent(0,0))
			ERRORBOX("Failed to release OpenGL rendering context");
		if (!wglDeleteContext(m_HGLRC))
			ERRORBOX("wglDeleteContext failed");
		m_HGLRC = 0;
	}
	
	// Release the device context.
	if (m_HDC && !ReleaseDC(m_HWND, m_HDC))
	{
		ERRORBOX("ReleaseDC failed");
		m_HDC = 0;
	}
	LeaveCriticalSection(&g_cs);
}

// Accessor for the current render state.
//
// Returns:					The current render state.
PI_Render::RenderState PI_Render::GetState(void) const
{
	EnterCriticalSection(&g_cs);
	RenderState rs = state;
	LeaveCriticalSection(&g_cs);
	return rs;
}

// BEGIN PRIVATE MEMBER FUNCTIONS

// Set initial OpenGL states.
bool PI_Render::InitOpenGL(void)
{
	// Extensions
	const char *extstr = (const char *)glGetString(GL_EXTENSIONS);
	if (!strstr(extstr, "ARB_multitexture"))
		return false;
	glActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREPROC)wglGetProcAddress("glActiveTextureARB");
	glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC)wglGetProcAddress("glMultiTexCoord2f");

	// Back buffer clear color
	glClearColor(0,0,0,1);

	// Depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Texture stage 1
	glActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	// Textures stage 0
	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	// Culling
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// Blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Stencil
	glClearStencil(0);

	// Necessary for lighting the world geometry correctly.
	glEnable(GL_COLOR_MATERIAL);

	UpdateViewport();
	return true;
}

// Log some information about the current OpenGL connection.
void PI_Render::LogGLConnection(void) const
{
	const char *theString;
	PI_Logger &logger = PI_Logger::GetInstance();

	logger << "OpenGL connection info:\n";

	// Who made it?
	theString = (char *)glGetString(GL_VENDOR);
	logger << "\tGL_VENDOR\t" << theString << '\n';

	// What's it called?
	theString = (char *)glGetString(GL_RENDERER);
	logger << "\tGL_RENDERER\t" << theString << '\n';

	// What version?
	theString = (char *)glGetString(GL_VERSION);
	logger << "\tGL_VERSION\t" << theString << '\n';

	// Get some GLU info.
	theString = (char *)gluGetString(GLU_VERSION);
	logger << "\tGLU_VERSION\t" << theString << '\n';

	GLint texUnits;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &texUnits);
	logger << "\t\t\t" << texUnits << " texture units\n\n";
}

// Log any OpenGL errors.
//
// In:		function		What function that called this one.
void PI_Render::LogOpenGLErrors(const char *function) const
{
	GLenum err;
	PI_Logger &logger = PI_Logger::GetInstance();
	if ((err = glGetError()) != GL_NO_ERROR)
	{
		logger << "ERROR " << function << "() - ";
		if (GL_INVALID_ENUM == err)
			logger << "glGetError() returned GL_INVALID_ENUM\n";
		else if (GL_INVALID_VALUE == err)
			logger << "glGetError() returned GL_INVALID_VALUE\n";
		else if (GL_INVALID_OPERATION == err)
			logger << "glGetError() returned GL_INVALID_OPERATION\n";
		else if (GL_STACK_OVERFLOW == err)
			logger << "glGetError() returned GL_STACK_OVERFLOW\n";
		else if (GL_STACK_UNDERFLOW == err)
			logger << "glGetError() returned GL_STACK_UNDERFLOW\n";
		else if (GL_OUT_OF_MEMORY == err)
			logger << "glGetError() returned GL_OUT_OF_MEMORY\n";
	}
}

// Update the renderer's viewport.
void PI_Render::UpdateViewport(void)
{
	RECT rClient;
	GetClientRect(m_HWND, &rClient);
	m_width = rClient.right;
	m_height = rClient.bottom;

	if (!m_width || !m_height)
		return;

	// Set the viewport and calculate the projection matrix.
	glViewport(0,0,m_width, m_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, float(m_width) / m_height, 1.0, 650);
	glGetFloatv(GL_PROJECTION_MATRIX, projectionMat);

	// Reset the modelview matrix.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// Apply the camera and build its frustum planes.
void PI_Render::ApplyCamera(void)
{
	EnterCriticalSection(&g_cs);
	if (!pActiveCam) {
		LeaveCriticalSection(&g_cs);
		return;
	}

	glLoadIdentity();	
	gluLookAt(pActiveCam->pos.x, pActiveCam->pos.y, pActiveCam->pos.z,
			  pActiveCam->target.x, pActiveCam->target.y, pActiveCam->target.z,
			  pActiveCam->up.x, pActiveCam->up.y, pActiveCam->up.z);

	// Combine the modelview and projection matricies, and use the result to extract the frustum planes.
	PI_Mat44 modelviewMat, worldSpaceMat;
	glGetFloatv(GL_MODELVIEW_MATRIX, modelviewMat);
	worldSpaceMat = projectionMat * modelviewMat;

	pActiveCam->frustumPlanes[PlaneLeft].normal.x = worldSpaceMat.mat[3] + worldSpaceMat.mat[0];
	pActiveCam->frustumPlanes[PlaneLeft].normal.y = worldSpaceMat.mat[7] + worldSpaceMat.mat[4];
	pActiveCam->frustumPlanes[PlaneLeft].normal.z = worldSpaceMat.mat[11] + worldSpaceMat.mat[8];
	pActiveCam->frustumPlanes[PlaneLeft].d = worldSpaceMat.mat[15] + worldSpaceMat.mat[12];

	pActiveCam->frustumPlanes[PlaneRight].normal.x = worldSpaceMat.mat[3] - worldSpaceMat.mat[0];
	pActiveCam->frustumPlanes[PlaneRight].normal.y = worldSpaceMat.mat[7] - worldSpaceMat.mat[4];
	pActiveCam->frustumPlanes[PlaneRight].normal.z = worldSpaceMat.mat[11] - worldSpaceMat.mat[8];
	pActiveCam->frustumPlanes[PlaneRight].d = worldSpaceMat.mat[15] - worldSpaceMat.mat[12];

	pActiveCam->frustumPlanes[PlaneBottom].normal.x = worldSpaceMat.mat[3] + worldSpaceMat.mat[1];
	pActiveCam->frustumPlanes[PlaneBottom].normal.y = worldSpaceMat.mat[7] + worldSpaceMat.mat[5];
	pActiveCam->frustumPlanes[PlaneBottom].normal.z = worldSpaceMat.mat[11] + worldSpaceMat.mat[9];
	pActiveCam->frustumPlanes[PlaneBottom].d = worldSpaceMat.mat[15] + worldSpaceMat.mat[13];
	
	pActiveCam->frustumPlanes[PlaneTop].normal.x = worldSpaceMat.mat[3] - worldSpaceMat.mat[1];
	pActiveCam->frustumPlanes[PlaneTop].normal.y = worldSpaceMat.mat[7] - worldSpaceMat.mat[5];
	pActiveCam->frustumPlanes[PlaneTop].normal.z = worldSpaceMat.mat[11] - worldSpaceMat.mat[9];
	pActiveCam->frustumPlanes[PlaneTop].d = worldSpaceMat.mat[15] - worldSpaceMat.mat[13];

	pActiveCam->frustumPlanes[PlaneNear].normal.x = worldSpaceMat.mat[3] + worldSpaceMat.mat[2];
	pActiveCam->frustumPlanes[PlaneNear].normal.y = worldSpaceMat.mat[7] + worldSpaceMat.mat[6];
	pActiveCam->frustumPlanes[PlaneNear].normal.z = worldSpaceMat.mat[11] + worldSpaceMat.mat[10];
	pActiveCam->frustumPlanes[PlaneNear].d = worldSpaceMat.mat[15] + worldSpaceMat.mat[14];

	pActiveCam->frustumPlanes[PlaneFar].normal.x = worldSpaceMat.mat[3] - worldSpaceMat.mat[2];
	pActiveCam->frustumPlanes[PlaneFar].normal.y = worldSpaceMat.mat[7] - worldSpaceMat.mat[6];
	pActiveCam->frustumPlanes[PlaneFar].normal.z = worldSpaceMat.mat[11] - worldSpaceMat.mat[10];
	pActiveCam->frustumPlanes[PlaneFar].d = worldSpaceMat.mat[15] - worldSpaceMat.mat[14];
	
	for (int i = 0; i < 6; ++i)
		pActiveCam->frustumPlanes[i].Normalize();

	// There may be a frame pending...
	if (vpRenderList->size())
		state = CommitFrameState;
	else
		state = ReadyState;
	LeaveCriticalSection(&g_cs);
}

// Load a list of assets - meshes, textures and world geometry.
//
// Returns				True if ALL assets are loaded.
bool PI_Render::LoadAssetList(void)
{
	EnterCriticalSection(&g_cs);
	bool foundAllAssets = true;
	unsigned int ignoreTexName;
	PI_Mesh ignoreMesh;
	const char *ext;

	const unsigned int NumAssets = (unsigned int)vpAssetList->size();
	for (unsigned int i = 0; i < NumAssets; i++)
	{
		// Get a pointer to the extension.
		ext = (*vpAssetList)[i].c_str();
		ext += (*vpAssetList)[i].length() - 3;

		if (!_stricmp(ext, "tga"))
		{
			// Targa texture.
			if (!LoadTarga((*vpAssetList)[i].c_str(), ignoreTexName, true))
				foundAllAssets = false;
			continue;
		}
		else if (!_stricmp(ext, "pim"))
		{
			// PigIron Mesh file.
			if (!LoadPIM((*vpAssetList)[i].c_str(), ignoreMesh))
				foundAllAssets = false;
			continue;
		}
		else if (!_stricmp(ext, "pwm"))
		{
			// PigIron World Mesh.
			if (!LoadWorldPIM((*vpAssetList)[i].c_str()))
				foundAllAssets = false;
			continue;
		}
	}

	state = ReadyState;
	LeaveCriticalSection(&g_cs);
	return foundAllAssets;
}

// Load a PigIron Mesh (PIM) file.
//
// In:		filename		Name of desired file, can be relative or absolute.
//
// Out:		out				The loaded PIM data.
//
// Returns					True if the file was loaded successfully (or is already loaded)
bool PI_Render::LoadPIM(const char *filename, PI_Mesh &out)
{
	string tempStr, filepath;
	unsigned int t = 0, v = 0, n = 0, numTris = 0, numTexs = 0;
	unsigned char tempFlags = 0;

	// The file path will be need when textures are loaded because the exporter doesn't
	// include paths in the texture filenames used by this mesh.
	filepath = filename;
	unsigned int index;
	if ((index = (unsigned int)filepath.find_last_of('\\')) != string::npos)
	{
		filepath.erase(index + 1);
	}
	else
		// No path.
		filepath.clear();

	// Temporary storage for the new mesh.
	PI_Triangle *pTris = 0;
	//PI_MeshNode node;
	PI_Mesh mesh;
	mesh.filename = filename;

	// Don't reload a PIM file that's already resident.
	const unsigned int NumMeshes = (unsigned int)vMeshes.size();
	for (unsigned int i = 0; i < NumMeshes; i++)
		if (mesh.filename == vMeshes[i].filename)
		{
			// Found it!
			out = vMeshes[i];
			return true;
		}

	// Attempt to open the file.
	ifstream fin(filename, ios_base::binary | ios_base::in);
	if (!fin.is_open())
		return false;

	// Get the number of nodes, then allocate them.
	fin.read((char *)&mesh.numNodes, sizeof(unsigned int));
	mesh.pNodes = new PI_MeshNode[mesh.numNodes];

	// Read 'em all in.
	for (n = 0; n < mesh.numNodes; n++)
	{
		// Read the node's name.
		getline(fin, mesh.pNodes[n].name);

		// How many textures does this node use?
		fin.read((char *)&numTexs, sizeof(unsigned int));

// TODO:: Load up more than just one texture. This code loads the last listed texture.
		if (numTexs > 0)
		{
			// Read in each filename.			
			for (t = 0; t < numTexs; t++)
				getline(fin, tempStr);

			// Add the path, and attempt to load.
			tempStr = filepath + tempStr;
			LoadTarga(tempStr.c_str(), mesh.pNodes[n].diffTexName, true);

// HACK:: Attempt to load a normal map by changing the last letter of the filename to an 'N',
//		  and trying to open the file.
// TODO:: Update the exporter to embed the normal map filename in the PIM file.
			tempStr[tempStr.find_last_of('.') - 1] = 'N';
			if (LoadTarga(tempStr.c_str(), mesh.pNodes[n].normalTexName, true))
				mesh.pNodes[n].flags |= NORMALMAPPED;
		}

		// Read in the flags.
		fin.read((char *)&tempFlags, sizeof(unsigned char));
		mesh.pNodes[n].flags |= tempFlags;

		// Read in the local transformation matrix.
		fin.read((char *)mesh.pNodes[n].ltm.mat, sizeof(float) * 16);
		
		// If the node isn't renderable, there won't be any geometry data in the file.
		if (!(mesh.pNodes[n].flags & RENDERABLE))
			continue;

		// Read all geometric data.
		fin.read((char *)&numTris, sizeof(unsigned int));
		pTris = new PI_Triangle[numTris];
		fin.read((char *)pTris, sizeof(PI_Triangle) * numTris);
	
		// Make sure the diffuse and normal texture names get stored per-triangle.
		for (unsigned int i = 0; i < numTris; ++i)
		{
			pTris[i].diffTex = mesh.pNodes[n].diffTexName;
			pTris[i].normTex = mesh.pNodes[n].normalTexName;
		}

		// Is this the world, or just an ordinary mesh?
		if (mesh.pNodes[n].flags & WORLD)
		{
			pWorld->AddToWorld(pTris, numTris);
		}
		else
		{
			// Build a display list for the mesh node.
			mesh.pNodes[n].displayList = glGenLists(1);
			glNewList(mesh.pNodes[n].displayList, GL_COMPILE);
			glBegin(GL_TRIANGLES);
			for (t = 0; t < numTris; t++)
			{
				for (v = 0; v < 3; ++v)
				{
					glNormal3f(pTris[t].normals[v].x, pTris[t].normals[v].y, pTris[t].normals[v].z);
					if (mesh.pNodes[n].flags & NORMALMAPPED)
					{
						glMultiTexCoord2f(GL_TEXTURE0, pTris[t].texCoord[v].u, pTris[t].texCoord[v].v);
						glMultiTexCoord2f(GL_TEXTURE1, pTris[t].texCoord[v].u, pTris[t].texCoord[v].v);
					}
					else
						glTexCoord2f(pTris[t].texCoord[v].u, pTris[t].texCoord[v].v);
					glVertex3f(pTris[t].verts[v].x, pTris[t].verts[v].y, pTris[t].verts[v].z);
				}
			}
			glEnd();
			glEndList();
		}

		// If this mesh casts shadows, we need to read in the edge data.
		if (mesh.pNodes[n].flags & CASTSHADOWS)
		{
			vector<PI_Edge> edgeVec;
			PI_Edge tempEdge;
			unsigned int numEdges = 0;
			fin.read((char *)&numEdges, sizeof(unsigned int));
			for (unsigned int i = 0; i < numEdges; i++)
			{
				fin.read((char *)&tempEdge, sizeof(PI_Edge));
				edgeVec.push_back(tempEdge);
			}

			// Add the edge vector into the master edge data map, using the node's display list as the key.
			edgeDataMap.insert(pair<unsigned int, vector<PI_Edge> >(mesh.pNodes[n].displayList, edgeVec));
		}

		// Read the bounding data.
		fin.read((char *)&mesh.pNodes[n].aabb_min, sizeof(PI_Vec3));
		fin.read((char *)&mesh.pNodes[n].aabb_max, sizeof(PI_Vec3));
		fin.read((char *)&mesh.pNodes[n].boundingRadius, sizeof(float));
		
		// Done processing this node.
		delete [] pTris;
	}

	// Successfully read in the PIM file.
	fin.close();
	out = mesh;
	vMeshes.push_back(mesh);
	return true;
}

// Load a PigIron Mesh (PIM) file as the world, and build the world tree.
//
// In:		filename		Name of desired file, can be relative or absolute.
//
// Returns					True if successful.
bool PI_Render::LoadWorldPIM(const char *filename)
{
	ULONGLONG start = GetTickCount64();

	pWorld->Clear();
	PI_Mesh temp;
	if (!LoadPIM(filename, temp))
		return false;
	pWorld->BuildWorldTree();

	PI_Logger &logger = PI_Logger::GetInstance();
	logger << "PI_Render::LoadWorldPIM() elapsed " << (GetTickCount64() - start) * 0.001f << " sec.\n";
	logger << "World tree contains " << pWorld->numWorldTris << " triangles split into " << pWorld->leafNodeCount << " leaf nodes ";
	logger << '(' << pWorld->nodeCount << " total nodes).\n\n";

	return true;
}

// Load a 24- or 32-bit uncompressed TARGA image file.
//
// In:		filename		Name of desired file, can be relative or absolute.
//			genMipMaps		Generate MIP maps?
//
// Out:		texName			Generated texture name.
//
// Returns					True if the file was loaded successfully (or is already loaded)
bool PI_Render::LoadTarga(const char *filename, unsigned int &texName, bool genMipMaps)
{
	PI_TexID texID = {filename, 0};
	GLenum format;
	unsigned int imageBytes;
	unsigned short width, height;
	unsigned char ID_Length, ColorMapType, ImageType, depth, *imageData, components;

	// Don't reload a texture that's already resident.
	const unsigned int NumTextures = (unsigned int)vTextures.size();
	for (unsigned int i = 0; i < NumTextures; i++)
		if (vTextures[i].filename == texID.filename)
		{
			texName = vTextures[i].texName;
			return true;
		}

	// Attempt to open the file.
	ifstream ifl(filename, ios::binary | ios::in);
	if (!ifl.is_open())
		return false;

	// Image ID length - should be 0.
	ifl.read((char *)&ID_Length, sizeof ID_Length);

	// Color Map Type - should be 0.
	ifl.read((char *)&ColorMapType, sizeof ColorMapType);

	// Image Type - should be 2 (Uncompressed true color image).
	ifl.read((char *)&ImageType, sizeof ImageType);

	// Make sure it's a type we can handle.
	if (ID_Length || ColorMapType || ImageType != 2)
	{
		// This format is unsupported.
		ifl.close();
		return false;
	}

	// Skip the color map specification field and X/Y origins.
	ifl.seekg(9, ios_base::cur);

	// Dimensions & color depth.
	ifl.read((char *)&width, sizeof width);
	ifl.read((char *)&height, sizeof height);
	ifl.read((char *)&depth, sizeof depth);

	// Only 24-bit and 32-bit supported.
	if (depth != 24 && depth != 32)
	{
		// This format is unsupported.
		ifl.close();
		return false;
	}
	else if (depth == 24)
		format = GL_BGR_EXT;
	else
		format = GL_BGRA_EXT;

	// Ignore the descriptor.
	ifl.seekg(1, ios_base::cur);

	// Image ID and Color Map fields should be zero bytes if we got this far,
	// so read in the actual image data.
	components = depth >> 3;
	imageBytes = width * height * components;
	imageData = new unsigned char[imageBytes];
	ifl.read((char *)imageData, imageBytes);
	ifl.close();

	// Generate a texture name and get ready to set it up.
	glGenTextures(1, &texID.texName);
	texName = texID.texName;
	vTextures.push_back(texID);
	glBindTexture(GL_TEXTURE_2D, texID.texName);
	activeTexStage0 = texID.texName;

	// Don't rely on the "defaults" really being default...
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Set the filters and build the texture.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (genMipMaps)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, components, width, height, format, GL_UNSIGNED_BYTE, imageData);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
	}

	// Success!
	delete [] imageData;
	return true;
}

// Bind a specific texture to the rendering context.
//
// In:		texName			The texture to bind.
void PI_Render::BindTexture2D(unsigned int texName)
{
	if (activeTexStage0 != texName)
	{
		glActiveTextureARB(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, activeTexStage0 = texName);
	}	
}

// Render the scene and display it.
void PI_Render::RenderScene(void)
{
	EnterCriticalSection(&g_cs);
	glFlush();

	// Clear the buffers and states.
	glClear(/*GL_COLOR_BUFFER_BIT |*/ GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	numTrisRendered = numNodesRendered = 0;
	pActiveDLight->ClearShadowVolume();


	// Set up texture stage 0 for DOT3 Normal mapping.
	// Use GL_DOT3_RGB_EXT to find the dot-product of (N.L), where N is 
	// stored in the normal map, and L is passed in as the PRIMARY_COLOR
	// using the standard glColor3f call.
	glActiveTextureARB(GL_TEXTURE0);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);      // Perform a Dot3 operation...
	glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_DOT3_RGB_EXT); 

	glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);           // between the N (of N.L) which is stored in a normal map texture...
	glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);

	glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT); // with the L (of N.L) which is stored in the vertex's diffuse color.
	glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);

	// Set up texture stage 1 to modulate in the diffuse texture.
	glActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	// Modulate the base texture by N.L calculated in STAGE 0.
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);  // Modulate...
	glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);

	glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PREVIOUS_EXT);
	glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);    // the color argument passed down from the previous stage (stage 0) with...
	
	glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
	glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);    // the texture for this stage.

	// Used to compute the light vector in model space during normal mapping.
	PI_Vec3 lightDirModel;

	// Render everything in the render list.
	unsigned int i;
	
	const unsigned int RenderListSize = vpRenderList->size();
	PI_RenderElement *pElement;
	for (i = 0; i < RenderListSize; ++i)
	{
		// Store a pointer to the current element to avoid redundant dereferencing ops.
		pElement = &(*vpRenderList)[i];

		// Apply the transform.
		glPushMatrix();
		glMultMatrixf(pElement->worldXform);

		// Load the normal texture, if necessary.
		if (activeTexStage0 != pElement->normTexName)
		{
			glActiveTextureARB(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, activeTexStage0 = pElement->normTexName);
		}
		
		// Load the diffuse texture, if necessary.
		if (activeTexStage1 != pElement->diffTexName)
		{
			glActiveTextureARB(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, activeTexStage1 = pElement->diffTexName);
		}
		
		// Calculate the light vector relative to the model.
		lightDirModel = pActiveDLight->dir;
		lightDirModel.TransposedRotate(pElement->worldXform);
		glColor3f(lightDirModel.x * 0.5f + 0.5f, lightDirModel.y * 0.5f + 0.5f, lightDirModel.z * 0.5f + 0.5f);			
		
		// Render!
		glCallList(pElement->displayList);

		// Does this node cast shadows?
		vector<PI_Edge> &vEdges = edgeDataMap.find(pElement->displayList)->second;
		if (vEdges.size())
			pActiveDLight->BuildShadowVolume(vEdges, pElement->worldXform);

		// Pop off the node's transform.
		glPopMatrix();
	}
	vpRenderList->mark_all_unused();

	// Shut down stage 1.
	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	activeTexStage1 = 0;

	// Set stage 0 back to normal.
	glActiveTextureARB(GL_TEXTURE0);
	glColor4f(RGBA_WHITE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);   
	glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
	glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);

	// Render the world with FF lighting.
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	// OpenGL requires the fourth 0 component for directional lights,
	// so make a copy of the main light vector.
	float glLightDir[4] = {pActiveDLight->dir.x, pActiveDLight->dir.y, pActiveDLight->dir.z, 0};
	glLightfv(GL_LIGHT0, GL_POSITION, glLightDir);
	RenderWorldTreeR(pWorld->pRootNode);
	glDisable(GL_LIGHTING);

	// Render the light's volumetric shadows.
	RenderShadows();

	// Render all particle emitters.
	glDisable(GL_DEPTH_TEST);
	const unsigned int NumEmitters = (unsigned int)vpEmitterList.size();
	for (i = 0; i < NumEmitters; ++i)
	{
		// Is the current emitter not in use?
		if (!vpEmitterList[i])
			// Yes, so skip it.
			continue;

		// Do we need to switch textures?
		if (vpEmitterList[i]->diffTex && activeTexStage0 != vpEmitterList[i]->diffTex)
			glBindTexture(GL_TEXTURE_2D, activeTexStage0 = vpEmitterList[i]->diffTex);

		RenderParticleEmitter(vpEmitterList[i]);

		// Mark this element as "unused".
		vpEmitterList[i] = 0;
	}

#if 1
	
	static float timeAccum = 0, frameRate = 0;
	static ULONGLONG timeStamp = GetTickCount(), frameCount = 0;

	++frameCount;
	timeAccum += (GetTickCount64() - timeStamp) / 1000.0f;
	if (timeAccum > 1.0f)
	{
		frameRate = frameCount / timeAccum;
		timeAccum = 0;
		frameCount = 0;
	}

	ostringstream out;
	out << " RenderListSize: " << RenderListSize << " FPS: " << frameRate;
	gui.DrawString(out, 10, 10, 24);
	timeStamp = GetTickCount64();

	gui.RenderGUI();

#endif

	glEnable(GL_DEPTH_TEST);

	// Put 'em on the glass.
	SwapBuffers(m_HDC);
	state = ReadyState;
	LeaveCriticalSection(&g_cs);
}

// Render all active shadow volumes.
void PI_Render::RenderShadows(void)
{
#if 1 // Volumetric shadow algorithm

	// Disable color & z-writes.
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glEnable(GL_STENCIL_TEST);

	// Stencil test always passes.
	glStencilFunc(GL_ALWAYS, 0, 255);

	// Draw front faces.
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	glInterleavedArrays(GL_V3F, 0, &pActiveDLight->vShadowVolume[0]);
	glDrawArrays(GL_QUADS, 0, pActiveDLight->shadowQuadCount * 4);

	// Draw back faces.
	glCullFace(GL_FRONT);
	glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
	glInterleavedArrays(GL_V3F, 0, &pActiveDLight->vShadowVolume[0]);
	glDrawArrays(GL_QUADS, 0, pActiveDLight->shadowQuadCount * 4);
	glCullFace(GL_BACK);

	// Draw the shadow.
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilFunc(GL_NOTEQUAL, 0, 255);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glPushMatrix();
	glLoadIdentity();
	
	// Switch to ortho.
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1,1,-1,1,-1,1);
	
	// Draw the a quad across the whole viewport.
	glMatrixMode(GL_MODELVIEW);
	glColor4f(0,0,0,0.5f);
	glBegin(GL_QUADS);
	glVertex2f(-1, 1);
	glVertex2f(-1, -1);
	glVertex2f(1, -1);
	glVertex2f(1, 1);
    glEnd();
	glColor4f(RGBA_WHITE);

	// Put perspective back.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// Done.
	glDepthMask(GL_TRUE);
	glDisable(GL_STENCIL_TEST);
#endif

#if 0 // Display the shadow volumes.
	glColor4f(RGB_GREEN, 0.5f);

	glBegin(GL_QUADS);
	for (unsigned int i = 0; i < pActiveDLight->shadowQuadCount; ++i)
		for (int v = 0; v < 4; ++v)
			glVertex3fv(pActiveDLight->vShadowVolume[i].verts[v]);
	glEnd();
	glColor4f(RGBA_WHITE);
#endif
}

// Render a particle emitter.
//
// In:		emit			The emitter to render.
void PI_Render::RenderParticleEmitter(const PI_ParticleEmitter *emit) const
{
	const vector<PI_ParticleEmitter::PI_Particle> &vParts = emit->vParticles;

	// Compute vectors for billboarding.
	PI_Vec3 normal = pActiveCam->pos - pActiveCam->target, scaled;		
	normal.Normalize();
	
	const PI_Vec3 upLeft = normal.Cross(pActiveCam->up) + pActiveCam->up;
	
	glBegin(GL_QUADS);
	const unsigned int NumParticles = (unsigned int)vParts.size();
	for (unsigned int i = 0; i < NumParticles; i++)
	{
		if (vParts[i].flags & PI_ParticleEmitter::PI_Particle::Active)
		{
			// Color the particle.
			if (emit->vColors.size() > 1)
				glColor4fv(emit->GetInterpolatedColor(vParts[i].life));

			scaled = upLeft * vParts[i].size;

			//glNormal3f(normal.x, normal.y, normal.z); LIGHTING IS DISABLED WHEN RENDERING PARTICLES!
			
			// Upper Left
			glTexCoord2f(0,1);
			glVertex3f(vParts[i].pos.x + scaled.x, vParts[i].pos.y + scaled.y, vParts[i].pos.z + scaled.z);

			// Lower Left
			glTexCoord2f(0,0);
			glVertex3f(vParts[i].pos.x + scaled.x, vParts[i].pos.y - scaled.y, vParts[i].pos.z - scaled.z);
			
			// Lower Right
			glTexCoord2f(1,0);
			glVertex3f(vParts[i].pos.x - scaled.x, vParts[i].pos.y - scaled.y, vParts[i].pos.z - scaled.z);
			
			// Upper Right
			glTexCoord2f(1,1);
			glVertex3f(vParts[i].pos.x - scaled.x, vParts[i].pos.y + scaled.y, vParts[i].pos.z + scaled.z);
		}
	}
	glColor4f(RGBA_WHITE);
	glEnd();
}

// Render the entire world tree.
// This function is recursive - pass the root node for the initial call.
void PI_Render::RenderWorldTreeR(const PI_WorldTree::PI_WorldTreeNode *n) const
{
	if (!n)
		return;

	// Compute cardinal axis dimension vectors for the node.
	PI_Vec3 R(n->max.x - n->min.x, 0, 0), S(0, n->max.y - n->min.y, 0), T(0, 0, n->max.z - n->min.z);
	float rEff;

	// Assume the root node is always visible.
	if (n != pWorld->pRootNode)
		for (unsigned char i = 0; i < 6; ++i)
		{
			// Compute the effective radius of the bounding box relative to each frustum plane.
			rEff = sqrt(pow(R.Dot(pActiveCam->frustumPlanes[i].normal), 2.0f) +
					pow(S.Dot(pActiveCam->frustumPlanes[i].normal), 2.0f) +
					pow(T.Dot(pActiveCam->frustumPlanes[i].normal), 2.0f));

			if (pActiveCam->frustumPlanes[i].DotHomogenous(n->center) <= -rEff)
				// The box is not visible, so bug out.
				return;
		}

	// Check if this is a leaf node where geometry is stored.
	if (!n->leftChild && !n->rightChild)
	{
		const unsigned int Size = (unsigned int)n->vRenderData.size();
		for (unsigned int i = 0; i < Size; ++i)
		{
			/*if (activeTexStage0 != n->vRenderData[i].normTexName)
			{
				glActiveTextureARB(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, activeTexStage0 = n->vRenderData[i].normTexName);
			}
			if (activeTexStage1 != n->vRenderData[i].diffTexName)
			{
				glActiveTextureARB(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, activeTexStage1 = n->vRenderData[i].diffTexName);
			}*/
			if (activeTexStage0 != n->vRenderData[i].diffTexName)
			{
				glActiveTextureARB(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, activeTexStage0 = n->vRenderData[i].diffTexName);
			}
			glCallList(n->vRenderData[i].displayList);
		}
		
		// Nothing else below this node.
		numTrisRendered += (unsigned int)n->vTris.size();
		numNodesRendered++;
		return;
	}
	
	// Not a leaf node, so recurse any children.
	if (n->leftChild)
		RenderWorldTreeR(n->leftChild);
	if (n->rightChild)
		RenderWorldTreeR(n->rightChild);
}

// Render some simple test geometry.
void PI_Render::RenderTestGeometry(void) const
{
#if 1
	glPushMatrix();
	glTranslatef(0,0,-10.0f);
	glBegin(GL_TRIANGLES);
	{
		glColor3f(1, 0, 0);
		glVertex3f(0,1,0);
		
		glColor3f(0, 1, 0);
		glVertex3f(-1.25f,-1,0);

		glColor3f(0, 0, 1);
		glVertex3f(1.25f,-1,0);	
	}
	glEnd();
	glPopMatrix();
#endif
}

// Find the first point where a ray intersects the world geometry.
// This function is recursive - pass the root node for the initial call.
bool PI_Render::FindFirstIntersectionR(const PI_Ray3 &ray, PI_Vec3 &intersectPoint, const PI_WorldTree::PI_WorldTreeNode *n) const
{
	// Where the ray intersects a plane.
	PI_Vec3 isectPlane;

	// Check the ray against all six AABB planes for this node.
	bool intersects = false;
	for (int i = 0; i < 6; i++)
	{
		if (ray.IntersectsPlane(n->aabb_planes[i], isectPlane))
		{
			// The ray pierces the plane, but does the intersection fall within the plane's endpoints?
			switch (i)
			{
			case PlaneLeft:
			case PlaneRight:
				// YZ Plane.
				if (isectPlane.y >= n->min.y && isectPlane.y <= n->max.y && isectPlane.z >= n->min.z && isectPlane.z <= n->max.z)
				{
					intersects = true;
					i = 6;
				}
				break;
			case PlaneBottom:
			case PlaneTop:
				// XZ Plane.
				if (isectPlane.x >= n->min.x && isectPlane.x <= n->max.x && isectPlane.z >= n->min.z && isectPlane.z <= n->max.z)
				{
					intersects = true;
					i = 6;
				}
				break;
			case PlaneNear:
			case PlaneFar:
				// XY Plane.
				if (isectPlane.x >= n->min.x && isectPlane.x <= n->max.x && isectPlane.y >= n->min.y && isectPlane.y <= n->max.y)
				{
					intersects = true;
					i = 6;
				}
				break;
			}
		}
	}

	if (!intersects)
		return false;
	else if (!n->leftChild && !n->rightChild)
	{
		// This is a leaf node, so look through all the geometry.
		const unsigned int NumTris = (unsigned int)n->vTris.size();
		for (unsigned int t = 0; t < NumTris; ++t)
			if (ray.IntersectsTriangle(n->vTris[t], intersectPoint))
				return true;
	}
	else if (n->leftChild && FindFirstIntersectionR(ray, intersectPoint, n->leftChild))
		// Found something in the left branch.
		return true;
	else if (n->rightChild && FindFirstIntersectionR(ray, intersectPoint, n->rightChild))
		// Found something in the right branch.
		return true;

	// Dead end.
	return false;
}

// Release all assets from memory.
void PI_Render::UnloadAllAssets(void)
{
	EnterCriticalSection(&g_cs);
	pWorld->Clear();
	UnloadAllTextures();
	UnloadAllStaticMeshes();
	state = ReadyState;
	LeaveCriticalSection(&g_cs);
}

// Release all textures from memory.
void PI_Render::UnloadAllTextures(void)
{
	const unsigned int NumTextures = (unsigned int)vTextures.size();
	for (unsigned int i = 0; i < NumTextures; i++)
		glDeleteTextures(1, &vTextures[i].texName);
	vTextures.clear();
}

// Release all static meshes from memory.
void PI_Render::UnloadAllStaticMeshes(void)
{
	const unsigned int NumMeshes = (unsigned int)vMeshes.size();
	for (unsigned int i = 0; i < NumMeshes; i++)
		for (unsigned int j = 0; j < vMeshes[i].numNodes; j++)
			glDeleteLists(vMeshes[i].pNodes[j].displayList, 1);
	vMeshes.clear();
	edgeDataMap.clear();
}

// DEPRICATED FUNCTIONS

// Add a mesh node to the render list.
//
// In:		mat				A world matrix where it should go.
//			node			Desired node to render.
//void PI_Render::AddToRenderList(const PI_Mat44 &mat, const PI_MeshNode &node)
//{
//	// Can the node even be rendered?
//	if (!(node.flags & RENDERABLE))
//		return;
//
//	vRenderList.push_back(PI_RenderElement(mat, node.displayList,
//											(node.flags & TEXTURED) ? node.diffTexName : 0,
//											(node.flags & NORMALMAPPED) ? node.normalTexName : 0));
//}

// Add a particle emitter to the render list.
//void PI_Render::AddToRenderList(const PI_ParticleEmitter &emitter)
//{
//	const unsigned int size = (unsigned int)vpEmitterList.size();
//	for (unsigned int i = 0; i < size; i++)
//	{
//		if (!vpEmitterList[i])
//		{
//			vpEmitterList[i] = &emitter;
//			return;
//		}
//	}
//
//	vpEmitterList.push_back(&emitter);
//}