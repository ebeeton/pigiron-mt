// Interface for the Player class.
//
// Copyright Evan Beeton 6/4/2005

#pragma once

#include "Entity.h"

class Player : public Entity
{
	friend class Game;

	// Particle emitters for the main cannon and secondary gun.
	//PI_ParticleEmitter cannonFlash, gunFlash;

	PI_Mesh playerMesh;

	PI_Mat44 cannonMat, turretMat,			// Transforms for the cannon and turret.
			 cannonFlashMat, gunFlashMat;	// Transforms for the cannon and secondary gun flash particle systems.

	PI_Vec2 mousePos;

	// Keep track of which nodes are which.
	const PI_MeshNode *pCannonNode, *pTurretNode, *pHullNode;

	// These nodes are for "attaching" particle systems.
	const PI_MeshNode *pCannonFlashNode, *pGunFlashNode;

	// How much is the turret rotated (radians)?
	float turretRot, // A little grease'll clear that right up...
		  speed;

	unsigned char flags;
	enum { LEFT_TREAD_FWD = 0x01, RIGHT_TREAD_FWD = 0x02, LEFT_TREAD_REV = 0x04, RIGHT_TREAD_REV = 0x08,
		   CANNON_KEY_DOWN = 0x10, GUN_KEY_DOWN = 0x20 };

public:

	// Default Constructor
	Player(void) : pCannonNode(0), pTurretNode(0), pHullNode(0), turretRot(0), flags(0)
				   //pCannonFlashNode(0), pGunFlashNode(0), cannonFlash(50), gunFlash(50)
	{ }

	// Initialize the player.
	//
	// Returns						True if successful.
	bool Init(void);

	// Update the player.
	//
	// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
	void Update(float deltaTime);

	// Process movement for the player.
	//
	// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
	void Move(float deltaTime);

	// Rotate the turret independently of the tank to avoid torque.
	void RotateTurret(void);

	// Aim the turret at the mouse cursor.
	//
	// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
	//				viewportX		Horizontal dimensions of the game window.
	//				viewportY		Vertical dimensions of the game window.
	void AimAtMouse(float deltaTime, int viewportX, int viewportY);

	// Process firing for the player.
	//
	// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
	void Fire(float deltaTime);

	// Add the player's mesh data to the render list.
	//
	// In:		vpList		The list to add to.
	virtual void AddToRenderList(reusable_vector<PI_RenderElement> &vpList);

	void SetMousePos(const PI_Vec2 &mouse)
	{
		mousePos = mouse;
	}

};