/*
 *  ParticleEngine.h
 *  FountainMusic
 *
 *  Copyright 2003-2011 Brian Moore
 *
 */

#ifndef _PARTICLEENGINE_H
#define _PARTICLEENGINE_H

#include "FMList.h"

//
// Particles
//
struct FMPoint3D
{
    float x;
    float y;
    float z;
}; 
typedef struct FMPoint3D FMPoint3D;


typedef struct FMSize3D
{
    float w;
    float h;
    float d;
} FMSize3D;


typedef struct FMRect3D
{
    FMPoint3D center;
    FMSize3D size;
} FMRect3D;

typedef struct FMColorRGB
{
    float r;
    float g;
    float b;
} FMColorRGB;

typedef struct Particle3D
{
    FMPoint3D position;
    FMPoint3D speed;
    FMPoint3D accel;
    
	float size;
    float mass;
    FMColorRGB color;
    
	float timeToDie;
	float yDimStart;
	float yDimEnd;
	
	float rotateSpeed;
	float rotation;
    
    int inUse;
    FMListNode *listNode;
} Particle3D;

float RandomFloatBetween(float a, float b);

void ResetParticle(Particle3D *particle);

// at some point, make the internals private and provide accessors:
//void SetParticleColor(Particle3D *particle, float r, float g, float b);
void GetParticleColor(Particle3D *particle, float *r, float *g, float *b);

void StepParticle(Particle3D *particle, float time);

//
// Particle Supply
//
#define MAX_PARTICLE_SUPPLY 6000

typedef struct ParticleSupply
{
    Particle3D particles[MAX_PARTICLE_SUPPLY];
    FMList *usedList;
    FMList *unusedList;
    int numberUsed;
} ParticleSupply;

ParticleSupply *NewParticleSupply();
void DeleteParticleSupply(ParticleSupply *supply);
void ResetParticleSupply(ParticleSupply *supply);

Particle3D *GetParticle(ParticleSupply *supply);

void PutParticleToUse(ParticleSupply *supply, Particle3D *particle);
void RemoveParticleFromUse(ParticleSupply *supply, Particle3D *particle);

void StepSupply(ParticleSupply *supply, float time);
int HandleSupplyOB(ParticleSupply *supply);
boolean_t HandleOB(ParticleSupply *supply, Particle3D *particle);

#endif
