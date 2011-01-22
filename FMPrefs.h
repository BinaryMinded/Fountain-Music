/*
 *  FMPrefs.h
 *  FountainMusic
 *
 *  Copyright 2004-2011 Brian Moore
 *
 */

#ifndef _FM_PREFS
#define _FM_PREFS

#include "FMDefs.h"

void FMSaveAllToPrefs(VisualPluginData *myData);
void FMRestoreAllFromPrefs(VisualPluginData *myData);

void FMSaveGravityToPrefs(VisualPluginData *myData);
void FMRestoreGravityFromPrefs(VisualPluginData *myData);

void FMSaveParticleSizeToPrefs(VisualPluginData *myData);
void FMRestoreParticleSizeFromPrefs(VisualPluginData *myData);

void FMSaveParticleSharpnessToPrefs(VisualPluginData *myData);
void FMRestoreParticleSharpnessFromPrefs(VisualPluginData *myData);

void FMSaveShowsParticleCounter(VisualPluginData *myData);
void FMRestoreShowsParticleCounter(VisualPluginData *myData);

void FMSaveShowsFPSCounter(VisualPluginData *myData);
void FMRestoreShowsFPSCounter(VisualPluginData *myData);

void FMSaveConstantTrackInfo(VisualPluginData *myData);
void FMRestoreConstantTrackInfo(VisualPluginData *myData);

void FMSaveColorSchemeInfo(VisualPluginData *myData);
void FMRestoreColorSchemeInfo(VisualPluginData *myData);

void FMSaveTextSizeInfo(VisualPluginData *myData);
void FMRestoreTextSizeInfo(VisualPluginData *myData);

#endif
