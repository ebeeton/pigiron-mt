// PigIron particle system implementation.
//
// Copyright Evan Beeton 6/12/2005

#include <fstream>
using std::ifstream;
using std::ofstream;
#include <algorithm>
using std::sort;

#include "PI_Particle.h"
#include "PI_Utils.h"


PI_ParticleEmitter::PI_ParticleEmitter(unsigned int maxParticles, EmitterType mode)
: posVar(0), dirVar(0), lifeVar(0), sizeVar(0), speedVar(0), genMode(mode),
  diffTex(0), liveParticles(0), maxLife(0), size(0), speed(0)
{
	vParticles.resize(maxParticles);
}

void PI_ParticleEmitter::Start(void)
{	
	const unsigned int NumParticles = (unsigned int)vParticles.size();
	for (unsigned int i = 0; i < NumParticles; i++)
	{
		// 99%	= 0.01 -> 1.99
		// 75%	= 0.25 -> 1.75
		// 50%	= 0.50 -> 1.50
		// 25%	= 0.75 -> 1.25
		// 1%	= 0.99 -> 1.01
		vParticles[i].life = maxLife * (RandomNum(100 + lifeVar, 100 - lifeVar) / 100.0f);
		vParticles[i].size = size * (RandomNum(100 + sizeVar, 100 - sizeVar) / 100.0f);
		vParticles[i].speed = speed * (RandomNum(100 + speedVar, 100 - speedVar) / 100.0f);

// TODO - work on randomizing initial position and direction.
		vParticles[i].pos = pos;
		vParticles[i].dir = dir;
	

		vParticles[i].flags |= PI_Particle::Active;
	}
	liveParticles = (unsigned int)vParticles.size();
}

void PI_ParticleEmitter::Update(float deltaTime)
{
	if (!liveParticles)
		return;

	const unsigned int NumParticles = (unsigned int)vParticles.size();
	for (unsigned int i = 0; i < NumParticles; i++)
	{
		// Check if the particle is dead.
		if (!(vParticles[i].flags & PI_Particle::Active) && genMode == Fountain)
		{
			// Respawn the particle.
			vParticles[i].life = maxLife * (RandomNum(100 + lifeVar, 100 - lifeVar) / 100.0f);
			vParticles[i].size = size * (RandomNum(100 + sizeVar, 100 - sizeVar) / 100.0f);
			vParticles[i].speed = speed * (RandomNum(100 + speedVar, 100 - speedVar) / 100.0f);
// TODO - work on randomizing initial position and direction.
			vParticles[i].pos = pos;
			vParticles[i].dir = dir;

			vParticles[i].flags |= PI_Particle::Active;
			++liveParticles;
		}
		else if (!(vParticles[i].flags & PI_Particle::Active))
		{
			// Particle is dead and not in fountain mode, so skip it.
			continue;
		}
		else if ((vParticles[i].life -= deltaTime) <= 0)
		{
			// Another one bites the dust.
			vParticles[i].flags &= ~PI_Particle::Active;
			--liveParticles;
			continue;
		}

		// The particle is alive, so update it.
		vParticles[i].pos += (vParticles[i].dir * vParticles[i].speed + force) * deltaTime;
	}
}

bool PI_ParticleEmitter::LoadPreset(const char *filename)
{
	const int TrashBufferLen = 100;
	char trash[TrashBufferLen];
	int temp;

	ifstream fin(filename);
	if (!fin.is_open())
		return false;

	fin.get(trash, TrashBufferLen);
	fin >> pos.x >> pos.y >> pos.z;
	while (fin.get() != '\n');
	fin.get();

	fin.get(trash, TrashBufferLen);
	fin >> temp;
	posVar = temp;
	while (fin.get() != '\n');
	fin.get();

	fin.get(trash, TrashBufferLen);
	fin >> dir.x >> dir.y >> dir.z;
	while (fin.get() != '\n');
	fin.get();

	fin.get(trash, TrashBufferLen);
	fin >> temp;
	dirVar = temp;
	while (fin.get() != '\n');
	fin.get();

	fin.get(trash, TrashBufferLen);
	fin >> force.x >> force.y >> force.z;
	while (fin.get() != '\n');
	fin.get();

	fin.get(trash, TrashBufferLen);
	fin >> maxLife >> temp;
	lifeVar = temp;
	while (fin.get() != '\n');
	fin.get();

	fin.get(trash, TrashBufferLen);
	fin >> size >> temp;
	sizeVar = temp;
	while (fin.get() != '\n');
	fin.get();

	fin.get(trash, TrashBufferLen);
	fin >> speed >> temp;
	speedVar = temp;
	while (fin.get() != '\n');
	fin.get();

	// Read the number of color keyframes.
	fin.get(trash, TrashBufferLen);
	fin >> temp;
	while (fin.get() != '\n');
	fin.get();

	if (temp > 0)
	{
		fin.get(trash, TrashBufferLen);

		// Read in the actual color keyframes.
		vColors.resize(temp);
		const unsigned int NumColors = (unsigned int)vColors.size();
		for (unsigned int i = 0; i < NumColors; i++)
		{
			fin >> vColors[i].val[PI_ColorKeyframe::Red]
				>> vColors[i].val[PI_ColorKeyframe::Green]
				>> vColors[i].val[PI_ColorKeyframe::Blue]
				>> vColors[i].val[PI_ColorKeyframe::Alpha]
				>> vColors[i].lifeTime;
			while (fin.get() != '\n');
		}
	}

	fin.close();
	return true;

}

void PI_ParticleEmitter::SavePreset(const char *filename) const
{
	ofstream fout(filename);

	fout << "Emitter position X Y Z\n";
	fout << pos.x << ' ' << pos.y << ' ' << pos.z << "\n\n";

	fout << "Emitter position variation %\n";
	fout << (int)posVar << "\n\n";

	fout << "Emitter direction normal X Y Z\n";
	fout << dir.x << ' ' << dir.y << ' ' << dir.z << "\n\n";

	fout << "Emitter direction variation %\n";
	fout << (int)dirVar << "\n\n";

	fout << "Emitter force\n";
	fout << force.x << ' ' << force.y << ' ' << force.z << "\n\n";
	
	fout << "Max life & Variation %\n";
	fout << maxLife << '\t' << (int)lifeVar << "\n\n";
	
	fout << "Size & Variation %\n";
	fout << size << '\t' << (int)sizeVar << "\n\n";

	fout << "Speed & Variation %\n";
	fout << speed << '\t' << (int)speedVar << "\n\n";

	fout << "Number of color keyframes\n";
	fout << (int)vColors.size() << "\n\n";

	fout << "R\tG\tB\tA\tParticle life\n";
	const unsigned int NumColors = (unsigned int)vColors.size();
	for (unsigned int i = 0; i < NumColors; i++)
	{
		fout << vColors[i].val[PI_ColorKeyframe::Red] << '\t'
			<< vColors[i].val[PI_ColorKeyframe::Green] << '\t'
			<< vColors[i].val[PI_ColorKeyframe::Blue] << '\t'
			<< vColors[i].val[PI_ColorKeyframe::Alpha] << '\t'
			<< vColors[i].lifeTime << '\n';
	}

	

	fout.close();
}


void PI_ParticleEmitter::AddColorKeyframe(float r, float g, float b, float a, unsigned char percentOfLife)
{
	if (percentOfLife > 100)
		percentOfLife = 100;

	vColors.push_back(PI_ColorKeyframe(r, g, b, a, maxLife * (percentOfLife / 100.0f)));
	sort(vColors.begin(), vColors.end());
}

PI_ParticleEmitter::PI_ColorKeyframe PI_ParticleEmitter::GetInterpolatedColor(float lifeTime) const
{
	// Look for the two closest keyframes.
	const PI_ColorKeyframe *prev, *next;
	prev = next = &vColors[0];

	const unsigned int NumColors = (unsigned int)vColors.size();
	for (unsigned int i = 0; i < NumColors; i++)
	{
		if (lifeTime < vColors[i].lifeTime)
		{
			prev = &vColors[i];
		}	
		else
		{
			next = &vColors[i];
			break;
		}
	}

	float normalizedTime = (lifeTime - prev->lifeTime) / (next->lifeTime - prev->lifeTime), omt = 1 - normalizedTime;

	// Get an interpolated color between the closest keyframes.
	return PI_ColorKeyframe(omt * prev->val[PI_ColorKeyframe::Red] + normalizedTime * next->val[PI_ColorKeyframe::Red],
							omt * prev->val[PI_ColorKeyframe::Green] + normalizedTime * next->val[PI_ColorKeyframe::Green],
							omt * prev->val[PI_ColorKeyframe::Blue] + normalizedTime * next->val[PI_ColorKeyframe::Blue],
							omt * prev->val[PI_ColorKeyframe::Alpha] + normalizedTime * next->val[PI_ColorKeyframe::Alpha],
							//omt * keyTime + time0to1 * rhs.keyTime);
							0);
}