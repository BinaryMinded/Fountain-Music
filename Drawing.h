/*
 *  Drawing.h
 *  FountainMusic
 *
 *  Copyright 2003, 2004, 2005, 2006, 2007 Brian Moore
 *
 */

#ifndef _FMDRAWING_H
#define _FMDRAWING_H

#import "FMDefs.h"

OSStatus SetupAGL(VisualPluginData *myData );
void CleanupAGL(VisualPluginData *myData);

void ShowFPSMeter(VisualPluginData *myData, boolean_t onFlag);
void ShowParticleMeter(VisualPluginData *myData, boolean_t onFlag);
void ShowMenu(VisualPluginData *myData);
void ShowMode(VisualPluginData *myData);
void ShowSharpnessLabel(VisualPluginData *myData);
void ShowTrackInfo(VisualPluginData *myData, boolean_t always);
void ShowColorTable(VisualPluginData *myData);
void ShowVersion(VisualPluginData *myData);

void UpdateAllDisplayItems(VisualPluginData *myData);
void UpdateFPSDisplayItem(VisualPluginData *myData);
void UpdateParticleDisplayItem(VisualPluginData *myData);

void InitDisplayItems(VisualPluginData *myData);
void StepDisplayItems(VisualPluginData *myData, float delta);
void LayoutDisplayItems(VisualPluginData *myData);

void RenderScene(VisualPluginData *myData);

void SetupContextRect(VisualPluginData *myData);

void Setup3DSystem(VisualPluginData *myData);
void Setup2DSystem(VisualPluginData *myData);

#ifdef ALBUM_ART
void CopyAlbumArtToTexture(VisualPluginData *myData, Handle artHandle, OSType artType);
#endif

#endif
