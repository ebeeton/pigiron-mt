// Game.h - The main game interface.
//
// Copyright Evan Beeton 6/4/2005

#pragma once

#include "PI_Render.h"
#include "PI_Logger.h"
#include "PI_GUI.h"

#include "Player.h"

class Game
{
public:
	
	enum GameState
	{
		StartupState,
		LoadState,
		PlayState,
		ShutdownState
	};

private:

	// This class is a Singleton.
	static Game m_instance;
	Game(const Game &rhs);
	Game &operator=(const Game &rhs);
	
	Game(void) : renderer(PI_Render::GetInstance()), logger(PI_Logger::GetInstance()), gui(PI_GUI::GetInstance()),
				 numOnscreenEntities(0), state(StartupState), curLevel(0)
	{ }

	// GUI elements.
	PI_GUIObject reticleGUI;

	// Game entities.
	PI_Camera camera;
	PI_DLight light;
	Player player;
	PI_Mesh terrainMesh;

	// All the world entities except the player.
	vector<Entity *> vEntity;

	// All the world entities (except the player) that are onscreen in a given frame.
	// These are kept in a separate vector to be sorted by distance from the player.
	vector<Entity *> vOnscreen;
	unsigned int numOnscreenEntities;

	// Everything to be rendered during the current frame.
	reusable_vector<PI_RenderElement> vRenderList;

	// The main subsystems.
	PI_Render &renderer;
	PI_Logger &logger;
	PI_GUI &gui;
	static HANDLE hRenderThread;

	GameState state;
	unsigned int curLevel;

public:
	// Singleton accessor.
	static Game &GetInstance(void) { return m_instance; }

	// Initialize the game.
	bool Init(void);

	// Load the current level assets and entities.
	//
	// Returns:		True if successful.
	bool LoadLevel(void);

	// Release the current level assets and entities.
	void UnloadLevel(void);

	// Update the game.
	//
	// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
	void Update(float deltaTime);

	// Release all resources and subsystems.
	void Shutdown(void);
	
	// Compare two entities based on their distance from the player.
	//
	// In:			e1
	//				e2
	//
	// Returns						True if e1 is closer to the player than e2.
	friend bool Compare(const Entity *e1, const Entity *e2);

	// Accessor for the rendering list.
	reusable_vector<PI_RenderElement> *GetRenderList(void) { return &vRenderList; }

private:
	// Cull world entities and update what's onscreen.
	//
	// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
	void UpdateOnscreenEntities(float deltaTime);
};
