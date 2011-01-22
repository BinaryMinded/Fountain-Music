/*
 *  FountainModes.h
 *  FountainMusic
 *
 *  Copyright 2003-2011 Brian Moore
 *
 */

#ifndef _FMFOUNTAINMODES_H
#define _FMFOUNTAINMODES_H

#include "iTunesVisualAPI.h"
#include "ParticleEngine.h"

typedef enum 
{
    FMRadarFountainMode = 0,
    FMAlternatingFountainMode,
    FMSolidFountainMode,
    FMDualRadarFountainMode,
    FMWackyClockFountainMode,
    FMSpiralFountainMode,
    FMStarfishFountainMode,
	FMRoseFountainMode
} FMFountainMode;

typedef struct FMFountainModeData *FMFountainModeDataRef;

FMFountainModeDataRef FMFountainModeCreateData();
void FMFountainModeFreeData(FMFountainModeDataRef fmData);

FMFountainMode FMFountainModeGetCurrentMode(FMFountainModeDataRef fmData);
void FMFountainModeSetCurrentMode(FMFountainModeDataRef fmData, FMFountainMode inMode);

Boolean FMFountainModeIsLocked(FMFountainModeDataRef fmData);
void FMFountainModeSetLocked(FMFountainModeDataRef fmData, Boolean flag);

float FMFountainModeGetTime(FMFountainModeDataRef fmData);
void FMFountainModeSetTime(FMFountainModeDataRef fmData, float inTime);

float FMFountainModeGetParameter(FMFountainModeDataRef fmData);
void FMFountainModeSetParameter(FMFountainModeDataRef fmData, float inParam);

void FMFountainModeCopyModeName(FMFountainModeDataRef fmData, FMFountainMode inMode, char *dest, unsigned int maxLen);
void FMFountainModeSwitch(FMFountainModeDataRef fmData, FMFountainMode newMode);
void FMFountainModeStepAnimation(FMFountainModeDataRef fmData, float timeStep);

void FMFountainModeIncrement(FMFountainModeDataRef fmData);
void FMFountainModeDecrement(FMFountainModeDataRef fmData);

void FMFountainModeSetupParticle(FMFountainModeDataRef fmData, Particle3D *particle, float percentFreq, float percentAmp, int channelNum);

/*// old API
float RandomFountainModeParameter(FMFountainMode mode); // uses random()
FMFountainMode RandomFountainMode(); // uses random()

void InitFountainMode(VisualPluginData *myData);
void SwitchFountainMode(VisualPluginData *myData, FMFountainMode newMode);
void StepModeAnimation(VisualPluginData *myData, float timeDelta);

Boolean FountainModeShouldSwitch(VisualPluginData *myData);

void SetupParticleForSampleInfo(Particle3D *particle, VisualPluginData *myData, float percentFreq, float percentAmp, int channelNum);

void IncrementFountainMode(VisualPluginData *myData);
void DecrementFountainMode(VisualPluginData *myData);

*/

#endif
