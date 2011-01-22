/*
 *  FMPrefs.c
 *  FountainMusic
 *
 *  Copyright 2004-2011 Brian Moore
 *
 */

#include "FMPrefs.h"
#import "Drawing.h"

#define kFMGravityPrefKey "\pGravityMultiplierR2"
#define kFMParticleSizePrefKey "\pParticleSizeR2"
#define kFMParticleSharpnessPrefKey "\pParticleSharpnessR2"
#define kFMShowsFPSCounterPrefKey "\pShowsFPSCounter"
#define kFMShowsParticleCounterPrefKey "\pShowsParticleCounter"
#define kFMConstantTrackInfoPrefKey "\pConstantTrackInfo"
#define kFMColorSchemePrefKey "\pColorSchemeName"
#define kFMPrefVersionPrefKey "\pPrefVersion"
#define kFMTextSizePrefKey "\pTextSize"

#define kFMColorSchemeNameBufferSize 128

const NumVersion kFMPrefsCurrentVersion = {0x01, 0x00, finalStage, 0x00};

void FMSavePrefVersionToPrefs(VisualPluginData *myData);
NumVersion FMGetPrefVersionFromPrefs(VisualPluginData *myData);

#pragma mark -

void FMSaveAllToPrefs(VisualPluginData *myData)
{
	FMSaveGravityToPrefs(myData);
	FMSaveParticleSizeToPrefs(myData);
	FMSaveParticleSharpnessToPrefs(myData);
	FMSaveShowsParticleCounter(myData);
	FMSaveShowsFPSCounter(myData);
	FMSaveConstantTrackInfo(myData);
	FMSaveColorSchemeInfo(myData);
	FMSaveTextSizeInfo(myData);
	FMSavePrefVersionToPrefs(myData);
}

void FMRestoreAllFromPrefs(VisualPluginData *myData)
{
	FMRestoreGravityFromPrefs(myData);
	FMRestoreParticleSizeFromPrefs(myData);
	FMRestoreParticleSharpnessFromPrefs(myData);
	FMRestoreShowsParticleCounter(myData);
	FMRestoreShowsFPSCounter(myData);
	FMRestoreConstantTrackInfo(myData);
	FMRestoreColorSchemeInfo(myData);
	FMRestoreTextSizeInfo(myData);
}

#pragma mark -
void FMSavePrefVersionToPrefs(VisualPluginData *myData)
{
	OSErr err;
	NumVersion currVers = kFMPrefsCurrentVersion;

	err = PlayerSetPluginNamedData(myData->appCookie, 
							 myData->appProc,
							 kFMPrefVersionPrefKey,
							 &currVers,
							 sizeof(NumVersion));
							 
	if (err != noErr) printf("Foutain Music: Error %i saving version to prefs!\n", err);

}

NumVersion FMGetPrefVersionFromPrefs(VisualPluginData *myData)
{
	UInt32 size;
	OSErr err;
	NumVersion vers;
	
	err = PlayerGetPluginNamedData(myData->appCookie, 
						 myData->appProc,
						 kFMPrefVersionPrefKey,
						 &vers,
						 sizeof(NumVersion),
						 &size);
						
	if (err != noErr) printf("Fountain Music: Error %i getting version from preferences!\n", err);
	
	if (size > 0)
	{
		return vers;
	}
	else
	{
		return kFMPrefsCurrentVersion;
	}
}


#pragma mark -
void FMSaveGravityToPrefs(VisualPluginData *myData)
{
	OSErr err;
	float gravMult = GetGravityMultiplier(myData);

	err = PlayerSetPluginNamedData(myData->appCookie, 
							 myData->appProc,
							 kFMGravityPrefKey,
							 &gravMult,
							 sizeof(float));
							 
	if (err != noErr) printf("Foutain Music: Error %i saving gravity to prefs!\n", err);
}

void FMRestoreGravityFromPrefs(VisualPluginData *myData)
{
	UInt32 size;
	float tempVal;
	OSErr err;
	
	err = PlayerGetPluginNamedData(myData->appCookie, 
							 myData->appProc,
							 kFMGravityPrefKey,
							 &tempVal,
							 sizeof(float),
							 &size);

	if (err != noErr || size != sizeof(float)) 
    {
        printf("Fountain Music: Error %i getting gravity preferences! Using default.\n", err);
        
        SetGravityMultiplier(myData, 1.0);
    }
    else
    {
        SetGravityMultiplier(myData, tempVal);
    }
}

#pragma mark -

void FMSaveParticleSizeToPrefs(VisualPluginData *myData)
{
	OSErr err;
	float particleSize = GetParticleSize(myData);

	err = PlayerSetPluginNamedData(myData->appCookie, 
							 myData->appProc,
							 kFMParticleSizePrefKey,
							 &particleSize,
							 sizeof(float));
							 
	if (err != noErr) printf("Fountain Music: Error %i saving particle size to prefs!\n", err);	
}

void FMRestoreParticleSizeFromPrefs(VisualPluginData *myData)
{
	UInt32 size;
	float tempVal;
	OSErr err;
	
	err = PlayerGetPluginNamedData(myData->appCookie, 
							 myData->appProc,
							 kFMParticleSizePrefKey,
							 &tempVal,
							 sizeof(float),
							 &size);

	if (err != noErr || size != sizeof(float))
    {
        printf("Fountain Music: Error %i getting gravity preferences! Using default.\n", err);
        SetParticleSize(myData, 0.2);
    }
    else
    {
        SetParticleSize(myData, tempVal);
    }
}

#pragma mark -

void FMSaveParticleSharpnessToPrefs(VisualPluginData *myData)
{
	OSErr err;
	float sharpVal = GetParticleSharpness(myData);

	err = PlayerSetPluginNamedData(myData->appCookie, 
							 myData->appProc,
							 kFMParticleSharpnessPrefKey,
							 &sharpVal,
							 sizeof(float));
							 
	if (err != noErr) printf("Fountain Music: Error %i saving particle sharpness to prefs!\n", err);		
}

void FMRestoreParticleSharpnessFromPrefs(VisualPluginData *myData)
{
	UInt32 size;
	float tempVal;
	OSErr err;
	
	err = PlayerGetPluginNamedData(myData->appCookie, 
							 myData->appProc,
							 kFMParticleSharpnessPrefKey,
							 &tempVal,
							 sizeof(float),
							 &size);

	if (err != noErr || size != sizeof(float))
    {
        printf("Fountain Music: Error %i getting sharpness preferences! Using default.\n", err);
        SetParticleSharpness(myData, 0.0);
    }
    else
    {
        SetParticleSharpness(myData, tempVal);
	}
}

#pragma mark -

void FMSaveShowsParticleCounter(VisualPluginData *myData)
{
	OSErr err;
	
	err = PlayerSetPluginNamedData(myData->appCookie,
							 myData->appProc,
							 kFMShowsParticleCounterPrefKey,
							 &myData->displayNumParticles,
							 sizeof(Boolean));
							 
	if (err != noErr) printf("Fountain Music: Error %i setting shows particle counter data!\n", err);
}

void FMRestoreShowsParticleCounter(VisualPluginData *myData)
{
	Boolean tempFlag;
	UInt32 tempSize;
	OSErr err;
	
	err = PlayerGetPluginNamedData(myData->appCookie,
								   myData->appProc,
								   kFMShowsParticleCounterPrefKey,
								   &tempFlag,
								   sizeof(Boolean),
								   &tempSize);
								   
	if (err != noErr || tempSize != sizeof(Boolean))
    {
        printf("Fountain Music: Error %i getting shows particle counter data! Using default.\n", err);
        SetShowsParticleCounter(myData, FALSE);
    }
	else
    {
        SetShowsParticleCounter(myData, tempFlag);
	}
}

#pragma mark -

void FMSaveShowsFPSCounter(VisualPluginData *myData)
{
	OSErr err;
	
	err = PlayerSetPluginNamedData(myData->appCookie,
							 myData->appProc,
							 kFMShowsFPSCounterPrefKey,
							 &myData->displayFPS,
							 sizeof(Boolean));
							 
	if (err != noErr) printf("Fountain Music: Error %i setting shows FPS counter data!\n", err);
}

void FMRestoreShowsFPSCounter(VisualPluginData *myData)
{
	Boolean tempFlag;
	UInt32 tempSize;
	OSErr err;
	
	err = PlayerGetPluginNamedData(myData->appCookie,
								   myData->appProc,
								   kFMShowsFPSCounterPrefKey,
								   &tempFlag,
								   sizeof(Boolean),
								   &tempSize);
								   
	if (err != noErr || tempSize != sizeof(Boolean))
    {
        printf("Fountain Music: Error %i getting shows FPS counter data! Using default.\n", err);
        SetShowsFPSCounter(myData, FALSE);
    }
	else
    {
        SetShowsFPSCounter(myData, tempFlag);
	}
}

#pragma mark -

void FMSaveConstantTrackInfo(VisualPluginData *myData)
{
	OSErr err;
	
	err = PlayerSetPluginNamedData(myData->appCookie,
							 myData->appProc,
							 kFMConstantTrackInfoPrefKey,
							 &myData->alwaysShowTrackInfo,
							 sizeof(Boolean));
							 
	if (err != noErr) printf("Fountain Music: Error %i setting constant track info data!\n", err);

}

void FMRestoreConstantTrackInfo(VisualPluginData *myData)
{
	Boolean tempFlag;
	UInt32 tempSize;
	OSErr err;
	
	err = PlayerGetPluginNamedData(myData->appCookie,
								   myData->appProc,
								   kFMConstantTrackInfoPrefKey,
								   &tempFlag,
								   sizeof(Boolean),
								   &tempSize);
								   
	if (err != noErr || tempSize != sizeof(Boolean))
    {
        printf("Fountain Music: Error %i getting constant track info data!\n", err);
        SetConstantTrackInfo(myData, FALSE);
    }
	else
    {
        SetConstantTrackInfo(myData, tempFlag);
	}
}

#pragma mark -

void FMSaveColorSchemeInfo(VisualPluginData *myData)
{
	OSErr err;
	UInt8 stringBuffer[kFMColorSchemeNameBufferSize];
	CFStringRef saveString = CopyColorTableName(myData, myData->currTable);
	CFIndex buffSize;
	
	memset(stringBuffer, 0, kFMColorSchemeNameBufferSize);
	
	CFStringGetBytes(saveString, 
					 CFRangeMake(0, CFStringGetLength(saveString)),
					 kCFStringEncodingUTF8,
					 '?',
					 TRUE, 
					 stringBuffer, 
					 kFMColorSchemeNameBufferSize, 
					 &buffSize);
					 
	err = PlayerSetPluginNamedData(myData->appCookie,
							 myData->appProc,
							 kFMColorSchemePrefKey,
							 stringBuffer,
							 buffSize);
							 
	if (err != noErr) printf("Fountain Music: Error %i setting color table name data!\n", err);

}

void FMRestoreColorSchemeInfo(VisualPluginData *myData)
{
	UInt8 stringBuffer[kFMColorSchemeNameBufferSize];
	UInt32 tempSize;
	CFStringRef tableName;
	int tableNum = 0;
	OSErr err;
	
	memset(stringBuffer, 0, kFMColorSchemeNameBufferSize);
	
	err = PlayerGetPluginNamedData(myData->appCookie,
								   myData->appProc,
								   kFMColorSchemePrefKey,
								   stringBuffer,
								   kFMColorSchemeNameBufferSize,
								   &tempSize);
								   
								   
	if (err != noErr || tempSize <= 0)
    {
        printf("Fountain Music: Error %i getting color table preference data! Using default.\n", err);
        
        SetColorTableIndex(myData, 0);
    }
    else
	{
		tableName = CFStringCreateWithBytes(NULL, stringBuffer, tempSize, kCFStringEncodingUTF8, TRUE);
	
		if (tableName != NULL)
		{
			tableNum = FindColorTableByName(myData, tableName);
			
			CFRelease(tableName);
			
			if (tableNum == -1) tableNum = 0;
		}
	
		SetColorTableIndex(myData, tableNum);
	}
}

#pragma mark -

void FMSaveTextSizeInfo(VisualPluginData *myData)
{
	OSErr err;
	Float32 tempTextSize;
	
	tempTextSize = GetTextSize(myData);
	
	err = PlayerSetPluginNamedData(myData->appCookie,
								   myData->appProc,
								   kFMTextSizePrefKey,
								   &tempTextSize,
								   sizeof(Float32));
								   
	if (err != noErr) printf("Fountain Music: Error %i setting text size data!\n", err);
}

void FMRestoreTextSizeInfo(VisualPluginData *myData)
{
	OSErr err;
	Float32 tempTextSize;
	UInt32 actualDataSize;
	
	err = PlayerGetPluginNamedData(myData->appCookie,
								   myData->appProc,
								   kFMTextSizePrefKey,
								   &tempTextSize,
								   sizeof(Float32),
								   &actualDataSize);
								   
	if (err != noErr || actualDataSize != sizeof(Float32))
    {
        printf("Fountain Music: Error %i getting text size data! Using default.\n", err);
        
        SetTextSize(myData, 12.0);
    }
	else
    {
		SetTextSize(myData, tempTextSize);
	}
}

