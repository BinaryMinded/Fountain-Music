/*
 *  FMAudioProcessing.h
 *  FountainMusic
 *
 *  Copyright 2005-2011 Brian Moore
 *
 */

//#ifndef _FMAUDIOPROCESSING_H
//#define _FMAUDIOPROCESSING_H

#include "iTunesVisualAPI.h"

#define kFMNumDataChannels 2
#define kFMNumSpectrumEntries 256

typedef float FMAudioSpectrumData[kFMNumDataChannels][kFMNumSpectrumEntries];

typedef struct FMAudioBuffer *FMAudioBufferRef;


FMAudioBufferRef FMAudioBufferCreate(int sampleCount);
void FMAudioBufferFree(FMAudioBufferRef buff);
void FMAudioBufferZero(FMAudioBufferRef buff);

void FMAudioBufferInsertITSample(FMAudioBufferRef buff, UInt8 iTunesData[kVisualMaxDataChannels][kVisualNumSpectrumEntries]);

void FMAudioBufferComputeAverage(FMAudioBufferRef buff);
FMAudioSpectrumData *FMAudioBufferGetAverage(FMAudioBufferRef buff);

void FMAudioSubtractSpectrumData(FMAudioSpectrumData *a, FMAudioSpectrumData *b, FMAudioSpectrumData *result);
//#endif