// Implementation of the Player class.
//
// Copyright Evan Beeton 6/4/2005

#include "PI_Utils.h"
#include "Game.h"

#define MAX_FWD_SPEED 15
#define MAX_REV_SPEED -5
#define ACCEL_RATE 7
#define TURN_RATE 1.5f
#define FRICTION 0.2f
#define TURRET_TURN_RATE 3
#define TURRET_DEAD_ZONE 0.0625f

// Initialize the player.
//
// Returns						True if successful.
bool Player::Init(void)
{
	// Set up the particle systems.
	unsigned int particleTex;
	if (!renderer.GetTextureHandle("DATA\\particle.tga", particleTex))
		return false;
	/*cannonFlash.SetLife(0.5f, 20);
	cannonFlash.SetSize(0.5f, 50);
	cannonFlash.SetSpeed(2.0f, 100);
	cannonFlash.AddColorKeyframe(1, 1, 1, 1, 100);
	cannonFlash.AddColorKeyframe(1, 0.75f, 0, 1, 90);
	cannonFlash.AddColorKeyframe(1, 0.25f, 0, 1, 80);
	cannonFlash.AddColorKeyframe(0.25f, 0.25f, 0.25f, 0.75f, 50);
	cannonFlash.AddColorKeyframe(0.25f, 0.25f, 0.25f, 0, 0);
	cannonFlash.SavePreset("DATA\\cannonFlash.txt");*/

//	cannonFlash.SetDiffuseTexture(particleTex);
//	cannonFlash.LoadPreset("DATA\\cannonFlash.txt");
	//cannonFlash.SetEmitterType(Fountain);

	// Load up the player geometry.
	if (!renderer.GetMeshHandle("DATA\\Player.PIM", playerMesh))
		return false;

	// Look for the named nodes.
	for (int i = 0; i < playerMesh.GetNumNodes(); i++)
	{
		if (playerMesh[i].GetName() == "Hull")
			pHullNode = &playerMesh[i];
		else if (playerMesh[i].GetName() == "Cannon")
			pCannonNode = &playerMesh[i];
		else if (playerMesh[i].GetName() == "Turret")
			pTurretNode = &playerMesh[i];
		else if (playerMesh[i].GetName() == "CannonDummy")
			pCannonFlashNode = &playerMesh[i];
		else if (playerMesh[i].GetName() == "GunDummy")
			pGunFlashNode = &playerMesh[i];
	}
	
	// Make sure we found all the nodes.
	return (pTurretNode && pCannonNode && pHullNode && pCannonFlashNode && pGunFlashNode);
}

// Update the player.
//
// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
void Player::Update(float deltaTime)
{	
	// Reset state variables.
	flags = 0;

	// Get input for the player.
	if (KEYDOWN('A'))
		flags |= RIGHT_TREAD_FWD;
	else if (KEYDOWN('Z'))
		flags |= RIGHT_TREAD_REV;
	if (KEYDOWN('S'))
		flags |= LEFT_TREAD_FWD;
	else if (KEYDOWN('X'))
		flags |= LEFT_TREAD_REV;
	if (KEYDOWN(VK_LBUTTON))
		flags |= CANNON_KEY_DOWN;
	if (KEYDOWN(VK_RBUTTON))
		flags |= GUN_KEY_DOWN;
		
	// Fire and movement.
	Move(deltaTime);

	AimAtMouse(deltaTime, renderer.GetViewportWidth(), renderer.GetViewportHeight());
	RotateTurret();
	Fire(deltaTime);

	// Calculate the gun's matrix - just glue it to the front of the turret.
	cannonMat = turretMat * pCannonNode->GetLTM();
}

// Process movement for the player.
//
// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
void Player::Move(float deltaTime)
{
	PI_Mat44 rotMat;
	float deltaRot = 0;

	// Straight on ahead.
	if ((LEFT_TREAD_FWD & flags) && (RIGHT_TREAD_FWD & flags))
	{
		if ((speed += ACCEL_RATE * deltaTime) > MAX_FWD_SPEED)
			speed = MAX_FWD_SPEED;
	}
	// Backing up.
	else if ((LEFT_TREAD_REV & flags) && (RIGHT_TREAD_REV & flags))
	{
		if ((speed -= ACCEL_RATE * deltaTime) < MAX_REV_SPEED)
			speed = MAX_REV_SPEED;
	}
	// Rotate right.
	else if ((LEFT_TREAD_FWD & flags) && !(RIGHT_TREAD_REV & flags) ||
			 !(LEFT_TREAD_FWD & flags) && (RIGHT_TREAD_REV & flags))
	{
		rotMat.MakeRotationMatrix(worldMat.GetAxisY(), deltaRot = -TURN_RATE * deltaTime);
		worldMat *= rotMat;
	}
	// Hard rotate right.
	else if ((LEFT_TREAD_FWD & flags) && (RIGHT_TREAD_REV & flags))
	{
		rotMat.MakeRotationMatrix(worldMat.GetAxisY(), deltaRot = -TURN_RATE * 2 * deltaTime);
		worldMat *= rotMat;
	}
	// Rotate left.
	else if ((RIGHT_TREAD_FWD & flags) && !(LEFT_TREAD_REV & flags) ||
		     !(RIGHT_TREAD_FWD & flags) && (LEFT_TREAD_REV & flags))
	{
		rotMat.MakeRotationMatrix(worldMat.GetAxisY(), deltaRot = TURN_RATE * deltaTime);
		worldMat *= rotMat;
	}
	// Hard rotate left.
	else if ((RIGHT_TREAD_FWD & flags) && (LEFT_TREAD_REV & flags))
	{
		rotMat.MakeRotationMatrix(worldMat.GetAxisY(), deltaRot = TURN_RATE * 2 * deltaTime);
		worldMat *= rotMat;
	}

	// Gradually bring them to a stop if they're not trying to move.
	if (!(LEFT_TREAD_FWD & flags) && !(RIGHT_TREAD_FWD & flags) &&
		!(LEFT_TREAD_REV & flags) && !(RIGHT_TREAD_REV & flags))
	{
		// Dead zone to stop 'em completely.
		if (speed <= FRICTION && speed >= -FRICTION)
			speed = 0;
		else
			// Slow em down.
			speed += (speed > 0 ? -FRICTION : +FRICTION);		
	}
	
	// Forward!
	worldMat.TranslateLocalZ(speed * deltaTime);

	// Glide across the terrain.
	AlignToWorld(pHullNode->GetAABBMin(), pHullNode->GetAABBMax());
}

// Rotate the turret independently of the tank to avoid torque.
void Player::RotateTurret(void)
{
	// Calculate the turret's matrix such that it shares the hull's XZ plane,
	// but doesn't inherit the Y rotation.
	PI_Vec3 turretUp = worldMat.GetAxisY();
	turretMat.SetAxisX(turretUp.Cross(PI_Vec3(0,0,1)));
	turretMat.SetAxisY(turretUp);
	turretMat.SetAxisZ(turretMat.GetAxisX().Cross(turretUp));

	// The turret has its own rotation.
	PI_Mat44 rotMat(PI_Vec3(0,1,0), turretRot);	
	turretMat *= rotMat;

	// Glue the turret to the top of the hull in world space.
	PI_Mat44 temp = worldMat * pTurretNode->GetLTM();
	turretMat.SetTranslation(temp.GetTranslation());
}

// Aim the turret at the mouse cursor.
//
// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
//				viewportX		Horizontal dimensions of the game window.
//				viewportY		Vertical dimensions of the game window.
void Player::AimAtMouse(float deltaTime, int viewportX, int viewportY)
{
	// Compute a vector from the center of the viewport to the aimpoint (mouse coordinates).
	const int centerViewX = viewportX >> 1, centerViewY = viewportY >> 1;
	PI_Vec2 mouseToCenter(float(mousePos.x - centerViewX), float(mousePos.y - centerViewY));
	
	// Make sure the mouse isn't at the dead center of the screen...
	if (mouseToCenter.x && mouseToCenter.y)
	{
		if (mouseToCenter.magnitude() < viewportY >> 3)
		{
			PI_Vec2 temp = mouseToCenter;
			temp.normalize();
			temp *= float(viewportY >> 3);
			SetCursorPos(int(temp.x) + centerViewX, (int)temp.y + centerViewY);
		}

		PI_Vec3 aimVec(mouseToCenter.x, 0, mouseToCenter.y), turVec = turretMat.GetAxisX();
		turVec.y = 0;
		turVec.Normalize();
		aimVec.Normalize();

		// Which way should the turret rotate?
		float leftRight = turVec.Dot(aimVec);		
		if (leftRight > TURRET_DEAD_ZONE)
			turretRot += (TURRET_TURN_RATE * deltaTime);
		else if (leftRight < -TURRET_DEAD_ZONE)
			turretRot -= (TURRET_TURN_RATE * deltaTime);
	}
}

// Process firing for the player.
//
// In:			deltaTime		How much time has elapsed since the last update. (milliseconds)
void Player::Fire(float deltaTime)
{
	//if (cannonFlash.IsActive())
	//{
	//	cannonFlash.Update(deltaTime);
	//	renderer.AddToRenderList(cannonFlash);
	//}
	//else if (flags & CANNON_KEY_DOWN)
	//{
	//	// Set up the particle emitter.
	//	cannonFlash.SetDirectionNormal(cannonMat.GetAxisZ(), 10);
	//	cannonFlash.SetPosition((cannonMat * pCannonFlashNode->GetLTM()).GetTranslation(), 10);
	//	cannonFlash.SetForce(worldMat.GetAxisZ() * speed);
	//	cannonFlash.Start();
	//}
}

// Add the player's mesh data to the render list.
//
// In:		vpList		The list to add to.
void Player::AddToRenderList(reusable_vector<PI_RenderElement> &vpList)
{
	// Add all nodes to the render list.
	vpList.push_back(PI_RenderElement(worldMat, *pHullNode));
	vpList.push_back(PI_RenderElement(cannonMat, *pCannonNode));
	vpList.push_back(PI_RenderElement(turretMat, *pTurretNode));
}