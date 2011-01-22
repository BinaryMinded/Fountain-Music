/*
 *  FMConfigure.h
 *  FountainMusic
 *
 *  Copyright 2004-2011 Brian Moore
 *
 */

#include "FMDefs.h"

#ifndef FM_CONFIGURE_H
#define FM_CONFIGURE_H

OSStatus FMInitializeConfig(VisualPluginData *myData);
OSStatus FMRunConfig(VisualPluginData *myData);
OSStatus FMCloseConfig(VisualPluginData *myData);

void FMConfigSyncUI(VisualPluginData *myData);

#endif