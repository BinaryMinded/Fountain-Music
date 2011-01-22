/*
 *  ParticleEngine.c
 *  FountainMusic
 *
 *  Copyright 2003-2011 Brian Moore
 *
 */

#include "ParticleEngine.h"
#include "FMDefs.h"

#ifdef FM_WINDOWS_BUILD
// 
#else
#include <stdlib.h>
#endif

void ResetParticle(Particle3D *particle)
{
    particle->position.x = 0.0f;
    particle->position.y = 0.0f;
    particle->position.z = 0.0f;
    
    particle->speed.x = 0.0f;
    particle->speed.y = 0.0f;
    particle->speed.z = 0.0f;
    
    particle->accel.x = 0.0f;
    particle->accel.y = 0.0f;
    particle->accel.z = 0.0f;
    
    particle->mass = 1.0f;
    particle->color.r = 1.0f;
    particle->color.g = 1.0f;
    particle->color.b = 1.0f;
	
	particle->yDimStart = 0.0;
	particle->yDimEnd = -1.0;
	
	particle->size = 1.0f;//RandomFloatBetween(1.0, 3.0);
    
	particle->rotateSpeed = RandomFloatBetween(-6.0, 6.0);
	particle->rotation = 0.0;
	
    particle->timeToDie = 0.0f;
}

inline float LinearEuler(float firstDeriv, float deltaX)
{
	return firstDeriv*deltaX;
}

inline float ParabolicEuler(float firstDeriv, float secondDeriv, float deltaX)
{
	return firstDeriv*deltaX + (0.5*deltaX*deltaX*secondDeriv);
}

void StepParticle(Particle3D *particle, float time)
{
	// instead of using euler's method with just the first derivative
	// adapt euler's method to use the second derivative too (parabolas
	// instead of lines)

	particle->speed.x += LinearEuler(particle->accel.x, time);
	particle->speed.y += LinearEuler(particle->accel.y, time);
	particle->speed.z += LinearEuler(particle->accel.z, time);

	particle->position.x += ParabolicEuler(particle->speed.x, particle->accel.x, time);
	particle->position.y += ParabolicEuler(particle->speed.y, particle->accel.y, time);
	particle->position.z += ParabolicEuler(particle->speed.z, particle->accel.z, time);
	
	particle->rotation += ParabolicEuler(particle->rotateSpeed, 0.0, time);
	
	particle->timeToDie -= time;
}

void GetParticleColor(Particle3D *particle, float *r, float *g, float *b)
{
	float dimFactor = 1.0; 

	if (particle->position.y < particle->yDimStart)
	{
		float dimDist = particle->yDimStart - particle->yDimEnd;
		
		dimFactor = 1.0 - (particle->yDimStart - particle->position.y)/dimDist;
	}
	
	*r = particle->color.r*dimFactor;
	*g = particle->color.g*dimFactor;
	*b = particle->color.b*dimFactor;
}

ParticleSupply *NewParticleSupply()
{
    int i;
    ParticleSupply *newSupply = calloc(sizeof(ParticleSupply), 1);
    
    if (newSupply == NULL)
    {
        DEBUG_OUT("Fountain Music Error: newSupply could not be allocated in NewParticleSupply()!\n");
        return NULL;
    }
    
    newSupply->usedList = FMNewList();
    newSupply->unusedList = FMNewList();
    
    for (i=0; i<MAX_PARTICLE_SUPPLY; i++)
    {
        // create the linked list node for the particle
        // it'll be used like a puzzle piece
        newSupply->particles[i].listNode = FMNewListNode(&(newSupply->particles[i]));

        ResetParticle(&(newSupply->particles[i]));        
        RemoveParticleFromUse(newSupply, &(newSupply->particles[i]));
    }
    
    newSupply->numberUsed = 0;

    return newSupply; 
}

void DeleteParticleSupply(ParticleSupply *supply)
{
    FMDeleteList(supply->usedList);
    FMDeleteList(supply->unusedList); // hopefully there are no duplicates
    
    free(supply);
}

void ResetParticleSupply(ParticleSupply *supply)
{
	FMListEnumerator *usedEnumer;
	FMListNode *currNode;

	usedEnumer = FMNewListEnumerator(supply->usedList);
	
	while (currNode = FMListEnumeratorNext(usedEnumer))
	{
		FMListEnumeratorRemoveCurrent(usedEnumer);
		FMListPrependNode(supply->unusedList, currNode);
		
		((Particle3D *)(currNode->data))->inUse = false;
		
		supply->numberUsed--;
	}

	FMDeleteListEnumerator(usedEnumer);
	
	if (supply->numberUsed != 0) FDEBUG_OUT("Fountain Music error: %i nodes unaccounted for in ResetParticleSupply()!", supply->numberUsed);
	if (supply->usedList->head != NULL) DEBUG_OUT("Fountain Music error: head node remains in used list after supply reset!");
	if (supply->usedList->tail != NULL) DEBUG_OUT("Fountain Music error: tail node remains in used list after supply reset!");
	
	// yep, even on failure pretend everything's alright
	supply->numberUsed = 0;
	supply->usedList->head = NULL;
	supply->usedList->tail = NULL;
}

void PutParticleToUse(ParticleSupply *supply, Particle3D *particle)
{
    particle->inUse = true;
	
    FMListDetachNode(supply->unusedList, particle->listNode);
    FMListAppendNode(supply->usedList, particle->listNode);
    
    supply->numberUsed++;
}

void RemoveParticleFromUse(ParticleSupply *supply, Particle3D *particle)
{
    particle->inUse = false;
	
    FMListDetachNode(supply->usedList, particle->listNode);
    FMListPrependNode(supply->unusedList, particle->listNode);
    
    supply->numberUsed--;
}

Particle3D *GetParticle(ParticleSupply *supply)
{
    FMListNode *nodeTaken = FMListDetachHead(supply->unusedList);
    
    if (nodeTaken)
    {
        PutParticleToUse(supply, nodeTaken->data);
        return nodeTaken->data;
    }
    else
    {
        return NULL;
    }
}

void StepSupply(ParticleSupply *supply, float time)
{
    FMListEnumerator *particleEnum = FMNewListEnumerator(supply->usedList);
    FMListNode *currNode;
    
    while (currNode = FMListEnumeratorNext(particleEnum))
    {
        StepParticle(currNode->data, time);
    }
    
    FMDeleteListEnumerator(particleEnum);
}

boolean_t ParticleIsOB(ParticleSupply *supply, Particle3D *particle)
{
	if ( particle->position.y < particle->yDimEnd || particle->timeToDie < 0.0f)
    {
		return TRUE;
    }
	return FALSE;
}

int HandleSupplyOB(ParticleSupply *supply)
{
    int ct = 0;
	FMListEnumerator *enumer;
	FMListNode *currNode;
	Particle3D *currParticle;
	
	enumer = FMNewListEnumerator(supply->usedList);
    
    while (currNode = FMListEnumeratorNext(enumer))
    {
		currParticle = (Particle3D *)(currNode->data);
		
        if (ParticleIsOB(supply, currParticle))
		{
			ct++;
			
			RemoveParticleFromUse(supply, currParticle); 
			// this is OK because currParticle is always the current particle in the enumerator
			// but perhaps to be safer, i should store a pointer to it on another DuLL and then remove
			// those all later
			
			ResetParticle(currParticle);
		}
    }
	
	FMDeleteListEnumerator(enumer);
	
	return ct;
}
