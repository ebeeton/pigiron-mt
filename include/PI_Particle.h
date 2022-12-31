// PigIron particle system interface.
//
// Copyright Evan Beeton 6/12/2005

#pragma once

#include <vector>
using std::vector;

#include "PI_Math.h"

// Identifiers for controlling how particles are emitted.
// Fountain - continuous, particles are reborn as they die.
// Explosion - once, system stops when all particles are dead.
enum EmitterType { Fountain, Explosion };

class PI_ParticleEmitter
{
	friend class PI_Render;

	class PI_Particle
	{
	public:

		PI_Vec3 pos, dir;
		
		float life, size, speed;

		enum { Active = 1 };
		unsigned char flags;

		PI_Particle(void) : life(0), size(0), speed(0), flags(0)
		{ }
	};
	
	class PI_ColorKeyframe
	{
	public:

		enum {Red, Green, Blue, Alpha};
		float val[4], lifeTime;

		PI_ColorKeyframe(float r = 0, float g = 0, float b = 0, float a = 0, float time = 0) : lifeTime(time)
		{ 
			val[Red] = r;
			val[Green] = g;
			val[Blue] = b;
			val[Alpha] = a;
		}

		operator const float *(void) const { return val; }

		bool operator<(const PI_ColorKeyframe &rhs) const
		{
			return lifeTime > rhs.lifeTime;
		}
	};

	vector<PI_Particle> vParticles;
	vector<PI_ColorKeyframe> vColors;

	PI_Vec3 pos, dir, force;
	
	float maxLife, size, speed;

	unsigned int diffTex, liveParticles;

	EmitterType genMode;

	// 1 -> 99 - the percentage to randomly vary individual particle attributes.
	unsigned char posVar, dirVar, lifeVar, sizeVar, speedVar;

public:

	PI_ParticleEmitter(unsigned int maxParticles, EmitterType mode = Explosion);

	void Start(void);

	void Stop(void) { liveParticles = 0; }

	void Update(float deltaTime);

	bool LoadPreset(const char *filename);

	void SavePreset(const char *filename) const;

	void AddColorKeyframe(float r, float g, float b, float a, unsigned char percentOfLife);

	PI_ColorKeyframe GetInterpolatedColor(float lifeTime) const;

	unsigned int IsActive(void) const { return liveParticles; }

	void SetPosition(const PI_Vec3 &where, unsigned char variance) { pos = where; posVar = variance; }

	void SetDirectionNormal(const PI_Vec3 &norm, unsigned char variance) { dir = norm; dirVar = variance; }

	void SetForce(const PI_Vec3 &vec) { force = vec; }

	void SetLife(float max, unsigned char variance) { maxLife = max; lifeVar = variance; }

	void SetSize(float s, unsigned char variance) { size = s; sizeVar = variance; }

	void SetSpeed(float s, unsigned char variance) { speed = s; speedVar = variance; }

	void SetDiffuseTexture(unsigned int texName) { diffTex = texName; }

	void SetEmitterType(EmitterType mode) { genMode = mode; }

};