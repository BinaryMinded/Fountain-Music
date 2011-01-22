/*
 *  FountainModes.c
 *  FountainMusic
 *
 *  Copyright 2003-2011 Brian Moore
 *
 */

#include "FountainModes.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef FM_WINDOWS_BUILD
#include "iTunesAPI.h"
#else
#include <Carbon/Carbon.h>
#endif

#ifndef MIN
#define MIN(x,y) ((x)>(y)?(y):(x))
#endif

#define SPRAY_HEIGHT (23.0f)

#define kFMFountainModeCount 8
#define kFMFountainModeNameMaxLength 128

typedef struct FMFountainModeData
{
	FMFountainMode mode;
	Boolean modeLocked;
	float modeTime;
	float modeSwitchTime;
	float modeParameter;
	char modeNames[kFMFountainModeCount][kFMFountainModeNameMaxLength];
} FMFountainModeData;

float FMFountainModeRandomParameter(FMFountainMode mode);
float FMFountainModeSwitchTime(FMFountainModeDataRef fmData);
Boolean FMFountainModeShouldSwitch(FMFountainModeDataRef fmData);
FMFountainMode FMFountainModeRandomMode();

double asinq(double x, int quadrantOffset);
float FMGaussianProb(float mean, float stdDev, float value);
float FMGaussianRandom(float mean, float stdDev);

#pragma mark -

double asinq(double x, int quadrantOffset)
{
	double refAngle = asin(x);
	double theta = 0.0;
	
	switch (quadrantOffset%4)
	{
		case 0:
			theta = refAngle;
		break;
			
		case 1: 
			theta = M_PI-refAngle;
		break;
		
		case 2:
			theta = M_PI+refAngle;
		break;
		
		case 3:
			theta = 2.0*M_PI - refAngle;
		break;
	}
	
	theta += (quadrantOffset/4) * 2.0*M_PI;
	
	return theta;
}

float FMGaussianRandom(float mean, float stdDev)
{
	float randVal, prob;
	
	do
	{
		randVal = RandomFloatBetween(mean-4.0*stdDev, mean+4.0*stdDev);
		prob = RandomFloatBetween(0.0, 1.0);
	}
	while (prob > FMGaussianProb(mean, stdDev, randVal));
	
	return randVal;
}

#pragma mark -

FMFountainModeDataRef FMFountainModeCreateData()
{
	FMFountainModeDataRef fmData;
	
	fmData = calloc(1, sizeof(FMFountainModeData)); // this will zero fmData, including all its members
	
	fmData->mode = FMSolidFountainMode;
	fmData->modeLocked = FALSE;
	fmData->modeTime = 0.0;
	fmData->modeParameter = 0.0;
	
	strcpy(fmData->modeNames[FMRadarFountainMode], "Radar");
    strcpy(fmData->modeNames[FMAlternatingFountainMode], "Alternating");
    strcpy(fmData->modeNames[FMSolidFountainMode], "Solid");
    strcpy(fmData->modeNames[FMDualRadarFountainMode], "Dual Radar");
    strcpy(fmData->modeNames[FMWackyClockFountainMode], "Wings");
    strcpy(fmData->modeNames[FMSpiralFountainMode], "Spiral");
    strcpy(fmData->modeNames[FMStarfishFountainMode], "Starfish");
	strcpy(fmData->modeNames[FMRoseFountainMode], "Flower");

	// actually set the initial mode (and put fmData in consistent state)
	FMFountainModeSwitch(fmData, FMSolidFountainMode);
	
	return fmData;
}

void FMFountainModeFreeData(FMFountainModeDataRef fmData)
{
	free(fmData);
}

FMFountainMode FMFountainModeGetCurrentMode(FMFountainModeDataRef fmData)
{
	return fmData->mode;
}

void FMFountainModeSetCurrentMode(FMFountainModeDataRef fmData, FMFountainMode inMode)
{
	fmData->mode = inMode;
}

Boolean FMFountainModeIsLocked(FMFountainModeDataRef fmData)
{
	return fmData->modeLocked;
}

void FMFountainModeSetLocked(FMFountainModeDataRef fmData, Boolean flag)
{
	fmData->modeLocked = flag;
	
	if (flag == FALSE)
	fmData->modeSwitchTime = fmData->modeTime + FMFountainModeSwitchTime(fmData);
}

float FMFountainModeGetTime(FMFountainModeDataRef fmData)
{
	return fmData->modeTime;
}

void FMFountainModeSetTime(FMFountainModeDataRef fmData, float inTime)
{
	fmData->modeTime = inTime;
}

float FMFountainModeGetParameter(FMFountainModeDataRef fmData)
{
	return fmData->modeParameter;
}

void FMFountainModeSetParameter(FMFountainModeDataRef fmData, float inParam)
{
	fmData->modeParameter = inParam;
}

void FMFountainModeCopyModeName(FMFountainModeDataRef fmData, FMFountainMode inMode, char *dest, unsigned int maxLen)
{
	memset(dest, 0, maxLen);
	strncpy(dest, fmData->modeNames[inMode], MIN(maxLen, kFMFountainModeNameMaxLength));
}

void FMFountainModeSwitch(FMFountainModeDataRef fmData, FMFountainMode newMode)
{
	// set the mode
    fmData->mode = newMode;
    
    // random parameter
    fmData->modeParameter = FMFountainModeRandomParameter(newMode);
    
    // zero percent
    fmData->modeTime = 0.0f;
	
	fmData->modeSwitchTime = FMFountainModeSwitchTime(fmData);
}

float FMFountainModeRandomParameter(FMFountainMode mode)
{
	float x;
    switch (mode)
    {
        case FMRadarFountainMode:
            // radians per second
            return RandomFloatBetween(1.0, 2.0);
            break;
        
        case FMAlternatingFountainMode:
            // radians for a block
            // 2π / between 8 and 24
            return 2.0*M_PI / floorf(RandomFloatBetween(8.0, 24.0));
            break;
            
        case FMSolidFountainMode:
            return 1.0;
            break;
            
        case FMDualRadarFountainMode:
            // radians per second
            return RandomFloatBetween(1.0, 2.0);
            break;
            
        case FMWackyClockFountainMode:
            // radians per second
            return RandomFloatBetween(1.0, 2.0);
            break;
            
        case FMSpiralFountainMode:
            // number of spirals
            return RandomFloatBetween(2.0, 10.0);
            break;
            
        case FMStarfishFountainMode:
            // number of snakes (avoid 4, looks too much like a swastika)
			return random()%2 ? 3.0 : 5.0;
            break;
			
		case FMRoseFountainMode:
			// number of petals (sorta)
			x = (float)(3 + random()%4);
            return x/2.0;
			break;
            
        default:
            return 1.0;
            break;
    }
}

void FMFountainModeStepAnimation(FMFountainModeDataRef fmData, float timeStep)
{
	// increment percent
    fmData->modeTime += timeStep;
    
    // check below function
    if ( FMFountainModeShouldSwitch(fmData) && !fmData->modeLocked )
    {
        FMFountainMode randFountainMode;
        
        // choose a random one not the same as the current
        while ( (randFountainMode = FMFountainModeRandomMode()) == fmData->mode );
        
        // set it to the new random one
        FMFountainModeSwitch(fmData, randFountainMode);
    }
}

float FMFountainModeSwitchTime(FMFountainModeDataRef fmData)
{
	switch (fmData->mode)
    {
        case FMRadarFountainMode:
        
            // yes after x times around
            return 8.0*M_PI/fmData->modeParameter;
            
            break;
            
        case FMAlternatingFountainMode:
        
            return 15.0f;
            
            break;
            
        case FMSolidFountainMode:
        
            // x secs
            return 15.0f;
            
            break;
            
        case FMDualRadarFountainMode:
            
            // x times around
            return 8.0*M_PI/fmData->modeParameter;
                        
            break;
            
        case FMWackyClockFountainMode:
        
            // x times around
            return 8.0*M_PI/fmData->modeParameter;
            
            break;
            
        case FMSpiralFountainMode:
        case FMStarfishFountainMode:
		case FMRoseFountainMode:
            
            // x seconds
            return 15.0;
            
            break;
            
        default:
            // after x secs
            return 15.0f;
            
            break;
    }
}

Boolean FMFountainModeShouldSwitch(FMFountainModeDataRef fmData)
{
	return fmData->modeTime > fmData->modeSwitchTime;
}

FMFountainMode FMFountainModeRandomMode()
{
    static FMFountainMode modes[] = 
    {
        FMRadarFountainMode,
        FMAlternatingFountainMode,
        FMSolidFountainMode,
        FMDualRadarFountainMode,
        FMWackyClockFountainMode,
        FMSpiralFountainMode,
        FMStarfishFountainMode,
		FMRoseFountainMode
    };
    return modes[random()%kFMFountainModeCount];
}

void FMFountainModeIncrement(FMFountainModeDataRef fmData)
{
	int nextMode = fmData->mode+1;
    
    if (nextMode >= kFMFountainModeCount) nextMode = 0;
    FMFountainModeSwitch(fmData, nextMode);	
}

void FMFountainModeDecrement(FMFountainModeDataRef fmData)
{
	int nextMode = fmData->mode-1;
    
    if (nextMode < 0) nextMode = kFMFountainModeCount-1;
    FMFountainModeSwitch(fmData, nextMode);
}

void FMFountainModeSetupParticle(FMFountainModeDataRef fmData, Particle3D *particle, float percentFreq, float percentAmp, int channel)
{
	float angle, radius, masterRadius = 8.0f;
	
	particle->size = FMGaussianRandom(1.0, 0.20);//(1.0 - percentFreq)*4.0;
	if (particle->size <= 0.0) particle->size = 0.01;
	
	// radius is frequency
	radius = percentFreq*masterRadius;
        
    switch (fmData->mode)
    {
        case FMRadarFountainMode:
            // angle is based on animation percent
            angle = fmData->modeParameter * fmData->modeTime;
            
			break;
            
        case FMAlternatingFountainMode:
            // angle is random, but can't be in blocked area
            // parameter determines blocked area sizes
            while ( ((int)((angle=RandomFloatBetween(0.0, 2.0*M_PI)) / fmData->modeParameter))%2 == ((int)fmData->modeTime)%2 );
            
			break;
            
        case FMSolidFountainMode:
            // angle is random
            angle = RandomFloatBetween(0.0, 2.0*M_PI);
            
			break;
            
        case FMDualRadarFountainMode:
            // same as radar, but with two arms 180˚ from each other
            // angle is based on animation percent
            angle = ( (float)channel * M_PI) + fmData->modeParameter * fmData->modeTime;
            
			break;
            
        case FMWackyClockFountainMode:
            // similar to dual radar, but the two arms move opposite like wings
            // angle is based on animation percent
			if (channel) angle = fmData->modeParameter * fmData->modeTime;
            else angle = M_PI - fmData->modeParameter * fmData->modeTime;
            
			break;
            
        case FMSpiralFountainMode:
            // particle is placed on a sprial
            // angle = (2πrn) / c
            
			angle = 2*M_PI*fmData->modeParameter*percentFreq;
            break;
			
		case FMRoseFountainMode:
			// n petal rose?
			// 4n quadrants, evens for 0, odds for 1
			// channel + 2*(random()%(2*n))
			angle = asinq(percentFreq, 2*(random()%(int)(fmData->modeParameter*2.0)) + channel ) / fmData->modeParameter;
			
			break;
            
        case FMStarfishFountainMode:
            // angle = ([0, param-1]/param)*2π
            angle = 0.35*sin(3.0*(percentFreq*M_PI + fmData->modeTime)) + 2.0*M_PI*(random()%(int)fmData->modeParameter) / fmData->modeParameter;
            
			break;
            
        default:
            // angle is random
            angle = RandomFloatBetween(0.0, 2.0*M_PI);
			
			break;
    }

	particle->position.y = 0.0;
	particle->position.x = cos(angle) * radius;
	particle->position.z = sin(angle) * radius;

    
    float speedRadius, speedAngle;
    
    speedAngle = angle + RandomFloatBetween(-0.1, 0.1); 
    speedRadius = (percentFreq*2.2) + 0.8; // spray more horizontal at ends
    
    particle->speed.x = speedRadius*cos(speedAngle);
    particle->speed.z = speedRadius*sin(speedAngle);
    particle->speed.y = percentAmp*SPRAY_HEIGHT;
}

// old api
/*
#pragma mark -


float RandomFountainModeParameter(FMFountainMode mode)
{
	float x;
    switch (mode)
    {
        case FMRadarFountainMode:
            // radians per second
            return RandomFloatBetween(1.0, 2.0);
            break;
        
        case FMAlternatingFountainMode:
            // radians for a block
            // 2π / between 8 and 24
            
            return 2.0*M_PI / floorf(RandomFloatBetween(8.0, 24.0));
            break;
            
        case FMSolidFountainMode:
            return 1.0;
            break;
            
        case FMDualRadarFountainMode:
            // radians per second
            return RandomFloatBetween(1.0, 2.0);
            break;
            
        case FMWackyClockFountainMode:
            // radians per second
            return RandomFloatBetween(1.0, 2.0);
            break;
            
        case FMSpiralFountainMode:
            // number of spirals
            return RandomFloatBetween(2.0, 10.0);
            break;
            
        case FMStarfishFountainMode:
            // number of snakes
			return random()%2 ? 3.0 : 5.0;
            break;
			
		case FMRoseFountainMode:
			// number of petals (sorta)
			x = (float)(3 + random()%4);
            return x/2.0;
			break;
		  
            
        default:
            return 1.0;
            break;
    }
}

FMFountainMode RandomFountainMode()
{
    static FMFountainMode modes[] = 
    {
        FMRadarFountainMode,
        FMAlternatingFountainMode,
        FMSolidFountainMode,
        FMDualRadarFountainMode,
        FMWackyClockFountainMode,
        FMSpiralFountainMode,
        FMStarfishFountainMode,
		FMRoseFountainMode
    };
    return modes[rand()%NUM_FOUNTAIN_MODES];
}

void InitFountainMode(VisualPluginData *myData)
{
    // zero everything, set initial mode
    strcpy(myData->modeNames[FMRadarFountainMode], "Radar");
    strcpy(myData->modeNames[FMAlternatingFountainMode], "Alternating");
    strcpy(myData->modeNames[FMSolidFountainMode], "Solid");
    strcpy(myData->modeNames[FMDualRadarFountainMode], "Dual Radar");
    strcpy(myData->modeNames[FMWackyClockFountainMode], "Wings");
    strcpy(myData->modeNames[FMSpiralFountainMode], "Spiral");
    strcpy(myData->modeNames[FMStarfishFountainMode], "Starfish");
	strcpy(myData->modeNames[FMRoseFountainMode], "Flower");
    
    myData->modeLocked = false;
    myData->modeTime = 0.0f;
    myData->modeParam = 0.0f;
    myData->mode = FMSolidFountainMode;
    
    SwitchFountainMode(myData, FMSolidFountainMode);
}

void SwitchFountainMode(VisualPluginData *myData, FMFountainMode newMode)
{
    // set the mode
    myData->mode = newMode;
    
    // random parameter
    myData->modeParam = RandomFountainModeParameter(newMode);
    
    // zero percent
    myData->modeTime = 0.0f;
}
void StepModeAnimation(VisualPluginData *myData, float timeDelta)
{
    // increment percent
    myData->modeTime += timeDelta;
    
    // check below function
    if ( FountainModeShouldSwitch(myData) && !myData->modeLocked )
    {
        FMFountainMode randFountainMode;
        
        // choose a random one not the same as the current
        while ( (randFountainMode = RandomFountainMode()) == myData->mode );
        
        // set it to the new random one
        SwitchFountainMode(myData, randFountainMode);
    }
}

Boolean FountainModeShouldSwitch(VisualPluginData *myData)
{
    
    switch (myData->mode)
    {
        case FMRadarFountainMode:
        
            // yes after x times around
            return (myData->modeTime * myData->modeParam) / (2.0*M_PI) > 4.0;
            
            break;
            
        case FMAlternatingFountainMode:
        
            // yes after x switches
            return (int)floor(myData->modeTime / 2.0) > 7;
            
            break;
            
        case FMSolidFountainMode:
        
            // x secs
            return myData->modeTime > 15.0f;
            
            break;
            
        case FMDualRadarFountainMode:
            
            // x times around
            return (myData->modeTime * myData->modeParam) / (2.0*M_PI) > 4.0;
                        
            break;
            
        case FMWackyClockFountainMode:
        
            // x times around
            return (myData->modeTime * myData->modeParam) / (2.0*M_PI) > 4.0;
            
            break;
            
        case FMSpiralFountainMode:
        case FMStarfishFountainMode:
		case FMRoseFountainMode:
            
            // x seconds
            return myData->modeTime > 15.0;
            
            break;
            
        default:
            // yes after x secs
            return myData->modeTime > 15.0f;
            
            break;
    }
}


void SetupParticleForSampleInfo(Particle3D *particle, VisualPluginData *myData, float percentFreq, float percentAmp, int channel)
{
    float angle, radius, masterRadius = 8.0f;
	
	particle->size = FMGaussianRandom(1.0, 0.20);//(1.0 - percentFreq)*4.0;
	if (particle->size <= 0.0) particle->size = 0.01;
	
	// radius is frequency
	radius = percentFreq*masterRadius;
        
    switch (myData->mode)
    {
        case FMRadarFountainMode:
            // angle is based on animation percent
            angle = myData->modeParam * myData->modeTime;
            
			break;
            
        case FMAlternatingFountainMode:
            // angle is random, but can't be in blocked area
            // parameter determines blocked area sizes
            while ( ((int)((angle=RandomFloatBetween(0.0, 2.0*M_PI)) / myData->modeParam))%2 == ((int)myData->modeTime)%2 );
            
			break;
            
        case FMSolidFountainMode:
            // angle is random
            angle = RandomFloatBetween(0.0, 2.0*M_PI);
            
			break;
            
        case FMDualRadarFountainMode:
            // same as radar, but with two arms 180˚ from each other
            // angle is based on animation percent
            angle = ( (float)channel * M_PI) + myData->modeParam * myData->modeTime;
            
			break;
            
        case FMWackyClockFountainMode:
            // similar to dual radar, but the two arms move opposite like wings
            // angle is based on animation percent
			if (channel) angle = myData->modeParam * myData->modeTime;
            else angle = M_PI - myData->modeParam * myData->modeTime;
            
			break;
            
        case FMSpiralFountainMode:
            // particle is placed on a sprial
            // angle = (2πrn) / c
            
			angle = 2*M_PI*myData->modeParam*percentFreq;
            break;
			
		case FMRoseFountainMode:
			// n petal rose?
			// 4n quadrants, evens for 0, odds for 1
			// channel + 2*(random()%(2*n))
			angle = asinq(percentFreq, 2*(random()%(int)(myData->modeParam*2.0)) + channel ) / myData->modeParam;
			
			break;
            
        case FMStarfishFountainMode:
            // angle = ([0, param-1]/param)*2π
            angle = 0.35*sin(3.0*(percentFreq*M_PI + myData->modeTime)) + 2.0*M_PI*(random()%(int)myData->modeParam) / myData->modeParam;
            
			break;
            
        default:
            // angle is random
            angle = RandomFloatBetween(0.0, 2.0*M_PI);
			
			break;
    }

	particle->position.y = 0.0;
	particle->position.x = cos(angle) * radius;
	particle->position.z = sin(angle) * radius;

    
    float speedRadius, speedAngle;
    
    speedAngle = angle + RandomFloatBetween(-0.1, 0.1); 
    speedRadius = (percentFreq*2.2) + 0.8; // spray more horizontal at ends
    
    particle->speed.x = speedRadius*cos(speedAngle);
    particle->speed.z = speedRadius*sin(speedAngle);
    particle->speed.y = percentAmp*SPRAY_HEIGHT;
	
	particle->accel.y = myData->currGravity;
}


void IncrementFountainMode(VisualPluginData *myData)
{
    int nextMode = myData->mode+1;
    
    if (nextMode >= NUM_FOUNTAIN_MODES) nextMode = 0;
    SwitchFountainMode(myData, nextMode);
}

void DecrementFountainMode(VisualPluginData *myData)
{
    int nextMode = myData->mode-1;
    
    if (nextMode < 0) nextMode = NUM_FOUNTAIN_MODES-1;
    SwitchFountainMode(myData, nextMode);
}
*/
