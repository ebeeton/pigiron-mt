// Game.cpp - The main game implementation.
//
// Copyright Evan Beeton 6/4/2005

#include <fstream>
using std::ifstream;
using std::ios_base;
#include <algorithm>
using std::sort;
#include <process.h>

#include "Game.h"
#include "MasterEntityList.h"
#include "PI_Utils.h"

Game Game::m_instance;
HANDLE Game::hRenderThread = 0;
extern CRITICAL_SECTION g_cs;

unsigned int __stdcall spawnRenderThread(void *v)
{
	PI_Logger &logger = PI_Logger::GetInstance();
	PI_Render &renderer = PI_Render::GetInstance();
	PI_GUI &gui = PI_GUI::GetInstance();

	if (!renderer.Init())
	{
		ERRORBOX("PI_Render::Init() failed");
		_endthreadex(-1);
		return -1;
	}
	logger << "PI_Render::Init() successful.\n";

	if (!gui.Init())
	{
		ERRORBOX("PI_GUI::Init() failed");
		_endthreadex(-1);
		return -1;
	}
	logger << "PI_GUI::Init() successful.\n";

	// TEST:: Load the font.
	if (!gui.LoadFont("DATA\\stencil"))
	{
		ERRORBOX("PI_GUI::LoadFont() failed!");
		return false;
	}

	renderer.SetRenderList(Game::GetInstance().GetRenderList());

	// Main rendering loop - update returns false once a shutdown state is set.
	while (renderer.Update());
	renderer.Shutdown();

	_endthreadex(0);
	return 0;
}

// Initialize the game.
bool Game::Init(void)
{
	srand(GetTickCount());
	ShowCursor(FALSE);

	// Initialize all subsystems.
	hRenderThread = (HANDLE)_beginthreadex(0, 0, spawnRenderThread, 0, 0, 0);
	if (hRenderThread)
		logger << "Render thread spawned successfully.\n";

	// The game was succesfully initialized.
	logger << "Game::Init() successful!\n";
	state = LoadState;
	return true;
}

// Load the current level assets and entities.
//
// Returns:		True if successful.
bool Game::LoadLevel(void)
{
	// Load the level script.
	const int MaxLen = 1000;
	char levelScriptName[MaxLen];
	sprintf(levelScriptName, "DATA\\level%d.PLS", curLevel);
	ifstream fin(levelScriptName, /*ios_base::binary |*/ ios_base::in);
	if (!fin.is_open())
		return false;

	// Load the assets.
	unsigned int numAssets;
	(fin >> numAssets).get();
	vector<string> assetList(numAssets);
	for (unsigned int i = 0; i < numAssets; i++)
		getline(fin, assetList[i]);
	renderer.SetAssetList(&assetList);

	// Wait for the renderer to finish loading before moving on.
	while (renderer.GetState() == PI_Render::LoadAssetState);

#if 0 // TODO:: Update the entity script exporter.

	// Read all the entities in.
	PI_Mat44 worldMat;
	string entityName;
	while (getline(fin, entityName))
	{
		Entity *newEntity = 0;

		// Look for a match among all the entity types.
		/*if (!stricmp(entityName.c_str(), "Palm1.PIM"))
			newEntity = new PalmTree;*/

		// Did we find a match?
		if (newEntity)
		{
			// Prepare the world matrix, init the entity, and add it to the world entity vector.
			fin.read((char *)worldMat.mat, sizeof(float) * 16);
			newEntity->SetWorldMat(worldMat);
			if (!newEntity->Init())
				return false;
			vEntity.push_back(newEntity);
		}
		else
			// File is corrupted.
			return false;
	}
#endif
	
	// Init the player.
	if (!player.Init())
	{
		ERRORBOX("Player::Init() failed!");
		return false;
	}

	// Set up the reticle GUI element.
	unsigned int texName;
	if (!renderer.GetTextureHandle("DATA\\Reticle.tga", texName))
	{
		ERRORBOX("Failed to load DATA\\Reticle.tga!");
		return false;
	}
	reticleGUI.SetTextureName(texName);
	reticleGUI.SetDimensions(64, 64);
	reticleGUI.SetColor(0, 1.0f, 0.125f, 1.0f);

	// Lights, camera...
	light.SetDirection(0.33f, 0.75f, 0.33f);
	renderer.SetActiveDLight(&light);

	camera.MoveTo(PI_Vec3(0,50,30));
	camera.LookAt(PI_Vec3(0,0,0));
	renderer.SetActiveCam(&camera);
	renderer.SetUpdateCameraState();

	// All world entities should be ready to go.
	fin.close();
	state = PlayState;
	return true;
}

// Release the current level assets and entities.
void Game::UnloadLevel(void)
{
	renderer.SetUnloadState();
	const unsigned int NumEntities = (unsigned int)vEntity.size();
	for (unsigned int i = 0; i < NumEntities; ++i)
		delete vEntity[i];
	vEntity.clear();
	vOnscreen.clear();
}

// Update the game.
//
// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
void Game::Update(float deltaTime)
{
	switch(state)
	{
		case LoadState:
			LoadLevel();
			break;
	}

	// The player must be updated before the camera, as it tracks the player.
	POINT mouse;
	GetCursorPos(&mouse);
	PI_Vec2 mouseVec((float)mouse.x, (float)mouse.y);
	player.SetMousePos(mouseVec);
	player.Update(deltaTime);

	// Update the camera.
	PI_Vec3 vec = player.GetWorldTranslation();
	EnterCriticalSection(&g_cs);
	camera.LookAt(vec);
	vec.y += 33;
	vec.z += 33;
	camera.MoveTo(vec);
	LeaveCriticalSection(&g_cs);
	
	// The camera must be applied before any frustum culling can be performed.
	renderer.SetUpdateCameraState();

	UpdateOnscreenEntities(deltaTime);
	
	// Mark all the onscreen entity slots as "unused" - we're done with this frame.
	numOnscreenEntities = 0;	

	// Draw the GUI.
	mouseVec.y = renderer.GetViewportHeight() - mouseVec.y;
	reticleGUI.SetPosition(mouseVec);
	gui.DrawGUIObject(&reticleGUI);

	// Make sure the renderer isn't currently trying to draw a frame.
	while (renderer.GetState() != PI_Render::ReadyState);

	// Add everything onscreen to the render list, and tell the rendering thread to draw.
	EnterCriticalSection(&g_cs);
	player.AddToRenderList(vRenderList);
	renderer.CommitFrame();
	LeaveCriticalSection(&g_cs);
}

// Release all resources and subsystems.
void Game::Shutdown(void)
{
	// Unload all asset and entity data.
	UnloadLevel();

	state = ShutdownState;
	logger << "\nGame::Shutdown() called.\n";

	// Shut down all subsystems.
	gui.Shutdown();

	// Wait for the renderer to finish unloading.
	while (renderer.GetState() != PI_Render::ReadyState);
	renderer.SetShutdownState();
	
	// Now wait for the thread to exit.
	if (WaitForSingleObject(hRenderThread, INFINITE) != WAIT_FAILED)
		logger << "Render thread exited successfully.\n";
	if(!CloseHandle(hRenderThread))
		logger << "Failed to close Render thread handle!\n";
	
	logger.Shutdown();
}

// Cull world entities and update what's onscreen.
//
// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
void Game::UpdateOnscreenEntities(float deltaTime)
{
	unsigned int i = 0;

	// Cull the world entities into a separate vector containing only those onscreen.
	const unsigned int NumEntities = (unsigned int)vEntity.size();
	for ( ; i < NumEntities; ++i)
		if (camera.SphereInFrustum(vEntity[i]->GetWorldTranslation(), vEntity[i]->GetBoundingRadius()))
		{
			// Look for an "unused" spot in the onscreen vector.
			if (++numOnscreenEntities < vOnscreen.size())
				vOnscreen[numOnscreenEntities - 1] = vEntity[i];
			else
				// Make room!
				vOnscreen.push_back(vEntity[i]);
			
		}

	// Sort the onscreen entities by their distance to the player.
	sort(vOnscreen.begin(), vOnscreen.begin() + numOnscreenEntities, Compare);

	// Update everything onscreen.
	for (i = 0; i < numOnscreenEntities; ++i)
		// Make sure the current slot is "in use"..
		vOnscreen[i]->Update(deltaTime);

}

// Compare two entities based on their distance from the player.
//
// In:			e1
//				e2
//
// Returns						True if e1 is closer to the player than e2.
bool Compare(const Entity *e1, const Entity *e2)
{
	Game &g = Game::GetInstance();
	float dist1 = (e1->GetWorldTranslation() - g.player.GetWorldTranslation()).MagnitudeSquared(),
		  dist2 = (e2->GetWorldTranslation() - g.player.GetWorldTranslation()).MagnitudeSquared();
	return dist1 < dist2;
}