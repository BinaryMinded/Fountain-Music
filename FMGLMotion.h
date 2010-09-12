/*
 *  FMGLMotion.h
 *  FountainMusic
 *
 *  Copyright 2004, 2005, 2006 Brian Moore
 *
 */

#include <Carbon/Carbon.h>
#import <OpenGL/gl.h>

#ifndef _FM_GL_MOTION
#define _FM_GL_MOTION

typedef struct FMOpaqueGLMotionBuffer *FMGLMotionBufferRef;

// memory management
FMGLMotionBufferRef FMGLMotionNewBuffer(int samples, CGSize bufferSize);
void FMGLMotionBufferDelete(FMGLMotionBufferRef buffer);

// accessors
int FMGLMotionBufferSamples(FMGLMotionBufferRef buffer);
CGSize FMGLMotionBufferSize(FMGLMotionBufferRef buffer);

// operations
void FMGLMotionBufferAdd(FMGLMotionBufferRef buffer, CGPoint sourceOrigin);
void FMGLMotionBufferAverage(FMGLMotionBufferRef buffer);
void FMGLMotionBufferDraw(FMGLMotionBufferRef buffer, CGPoint sourceOrigin);

#endif