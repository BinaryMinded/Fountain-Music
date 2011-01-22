/*
 *  FMAudioProcessing.c
 *  FountainMusic
 *
 *  Copyright 2005-2011 Brian Moore
 *
 */

#include "FMAudioProcessing.h"

#ifndef MIN
#define MIN(x,y) (x>y?y:x)
#endif

typedef struct _FMAudioBuffer
{
    FMAudioSpectrumData *audioSamples;
    int headIndex;
    int sampleCount;
	FMAudioSpectrumData average;
} FMAudioBuffer;

FMAudioBufferRef FMAudioBufferCreate(int sampleCount)
{
	FMAudioBuffer *buff = calloc(1, sizeof(FMAudioBuffer));
	
	buff->audioSamples = calloc(sampleCount, sizeof(FMAudioSpectrumData));
	buff->headIndex = 0;
	buff->sampleCount = sampleCount;
	
	return (FMAudioBufferRef)buff;
}

void FMAudioBufferIncrementIndex(FMAudioBufferRef buff);

#pragma mark -

void FMAudioBufferFree(FMAudioBufferRef buff)
{
	FMAudioBuffer *buffPtr = (FMAudioBuffer *)buff;

	free(buffPtr->audioSamples);
	free(buff);
}

void FMAudioBufferZero(FMAudioBufferRef buff)
{
	FMAudioBuffer *buffPtr = (FMAudioBuffer *)buff;

	memset(buffPtr->average, 0, sizeof(FMAudioSpectrumData));
	memset(buffPtr->audioSamples, 0, buffPtr->sampleCount*sizeof(FMAudioSpectrumData));
}

void FMAudioBufferComputeAverage(FMAudioBufferRef buff)
{
	FMAudioBuffer *buffPtr = (FMAudioBuffer *)buff;
	int s, c, i;
	
	memset(buffPtr->average, 0, sizeof(FMAudioSpectrumData));
	
	for (s=0; s<buffPtr->sampleCount; s++)
	{
		for (c=0; c<kFMNumDataChannels; c++)
		{
			for (i=0; i<kFMNumSpectrumEntries; i++)
			{
				buffPtr->average[c][i] += buffPtr->audioSamples[s][c][i] / buffPtr->sampleCount;
			}
		}
	}
}

void FMAudioBufferInsertITSample(FMAudioBufferRef buff, UInt8 iTunesData[kVisualMaxDataChannels][kVisualNumSpectrumEntries])
{
	FMAudioBuffer *buffPtr = (FMAudioBuffer *)buff;
	int c, i;
	
	FMAudioBufferIncrementIndex(buff);
	
	// fill the new current sample
	for (c=0; c<MIN(kVisualMaxDataChannels, kFMNumDataChannels); c++)
	{
		for (i=0; i<MIN(kVisualNumSpectrumEntries, kFMNumSpectrumEntries); i++)
		{
			buffPtr->audioSamples[buffPtr->headIndex][c][i] = (float)iTunesData[c][i]/255.0f;
		}
	}
}

void FMAudioSubtractSpectrumData(FMAudioSpectrumData *a, FMAudioSpectrumData *b, FMAudioSpectrumData *result)
{
	int c, i;
	
	for (c=0; c<kFMNumDataChannels; c++)
	{
		for (i=0; i<kFMNumSpectrumEntries; i++)
		{
			(*result)[c][i] = (*a)[c][i] - (*b)[c][i];
		}
	}
}

void FMAudioBufferIncrementIndex(FMAudioBufferRef buff)
{
	FMAudioBuffer *buffPtr = (FMAudioBuffer *)buff;

	buffPtr->headIndex = (buffPtr->headIndex+1)%buffPtr->sampleCount;
}

FMAudioSpectrumData *FMAudioBufferGetAverage(FMAudioBufferRef buff)
{
	FMAudioBuffer *buffPtr = (FMAudioBuffer *)buff;
	
	return &buffPtr->average;
}
