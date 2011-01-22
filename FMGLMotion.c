/*
 *  FMGLMotion.c
 *  FountainMusic
 *
 *  Copyright 2004-2011 Brian Moore
 *
 */

#include "FMGLMotion.h"
#import <OpenGL/glu.h>
#import <OpenGL/glext.h>

// util
inline void FMTexRectf(GLuint texture, float texWidth, float texHeight, float x, float y, float width, float height)
{
	glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texture);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
			glVertex2f(x, y);
		glTexCoord2f(texWidth, 0.0); 
			glVertex2f(x+width, y);
		glTexCoord2f(texWidth, texHeight); 
			glVertex2f(x+width, y+height);
		glTexCoord2f(0.0, texHeight); 
			glVertex2f(x, y+height);
	glEnd();
}

typedef struct FMOpaqueGLMotionBuffer
{
	GLuint numSamples;
	GLuint *sampleTex;
	GLuint currHead;
	
	GLuint samplesRecorded;
	
	GLuint averageTex;
	GLuint tempTex;
	
	CGSize bufferSize;
} FMOpaqueGLMotionBuffer;

// memory management
FMGLMotionBufferRef FMGLMotionNewBuffer(int samples, CGSize bufferSize)
{
	FMGLMotionBufferRef newBuffer = malloc(sizeof(FMOpaqueGLMotionBuffer));

	newBuffer->numSamples = samples;
	newBuffer->sampleTex = calloc(newBuffer->numSamples, sizeof(GLuint *));
	newBuffer->currHead = 0;	
	newBuffer->samplesRecorded = 0;
	newBuffer->bufferSize = bufferSize;

	// create the texture objects
	glGenTextures(newBuffer->numSamples, newBuffer->sampleTex);
	glGenTextures(1, &(newBuffer->averageTex));
	glGenTextures(1, &(newBuffer->tempTex));

	return newBuffer;
}

void FMGLMotionBufferDelete(FMGLMotionBufferRef buffer)
{
	glDeleteTextures(buffer->numSamples, buffer->sampleTex);
	glDeleteTextures(1, &(buffer->averageTex));
	glDeleteTextures(1, &(buffer->tempTex));

	free(buffer);
}

// accessors
int FMGLMotionBufferSamples(FMGLMotionBufferRef buffer)
{
	return buffer->numSamples;
}

CGSize FMGLMotionBufferSize(FMGLMotionBufferRef buffer)
{
	return buffer->bufferSize;
}

// operations
void FMGLMotionBufferAdd(FMGLMotionBufferRef buffer, CGPoint sourceOrigin)
{
	if (buffer->samplesRecorded < buffer->numSamples) buffer->samplesRecorded++;
	if (buffer->currHead == 0) buffer->currHead = buffer->numSamples-1;
	else buffer->currHead = buffer->currHead-1;
	
	glBindTexture(GL_TEXTURE_RECTANGLE_EXT, buffer->sampleTex[buffer->currHead]);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGB8, 
					 sourceOrigin.x, sourceOrigin.y, 
					 buffer->bufferSize.width,
					 buffer->bufferSize.height, 0);
}

void FMGLMotionBufferAverage(FMGLMotionBufferRef buffer)
{
	int i;
	int w, h;
	GLfloat viewport[4];
	
	w = buffer->bufferSize.width;
	h = buffer->bufferSize.height;
	
	// save state
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	glEnable(GL_TEXTURE_RECTANGLE_EXT);
	
	// store the area i am about to draw over
	glBindTexture(GL_TEXTURE_RECTANGLE_EXT, buffer->tempTex);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGB8,
					 0, 0, w, h, 0);
	
	glBlendFunc(GL_ONE, GL_ZERO);
	
	glGetFloatv(GL_VIEWPORT, viewport);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, viewport[2], 0.0, viewport[3], 1.0, -1.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// clear the area for averaging
	glColor3f(1.0, 1.0, 1.0);
	glRectf(0.0, 0.0, w, h);
	
	// composite every frame into the area
	glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
	glBlendColor(1.0, 1.0, 1.0, 1.0/buffer->samplesRecorded);
	
	for (i=0; i < buffer->samplesRecorded; i++) 
	{
		glBlendColor(1.0, 1.0, 1.0, (buffer->samplesRecorded-i-1.0)/buffer->samplesRecorded);
		FMTexRectf(buffer->sampleTex[(buffer->currHead + i)%buffer->numSamples], w, h, 0.0, 0.0, w, h);
	}
	
	// copy the resulting image into the average texture
	glBindTexture(GL_TEXTURE_RECTANGLE_EXT, buffer->averageTex);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGB8,
					 0, 0, w, h, 0);
	
	glBlendFunc(GL_ONE, GL_ZERO);
	// restore what was there
	FMTexRectf(buffer->tempTex, w, h,
				0.0, 0.0, w, h);
	
	glDisable(GL_TEXTURE_RECTANGLE_EXT);
	
	// restore old state
	glPopAttrib();
}

void FMGLMotionBufferDraw(FMGLMotionBufferRef buffer, CGPoint origin)
{
	int w, h;
	GLfloat viewport[4];
	
	w = buffer->bufferSize.width;
	h = buffer->bufferSize.height;
	
	// save state
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	glEnable(GL_TEXTURE_RECTANGLE_EXT);
	
	glBlendFunc(GL_ONE, GL_ZERO);
	
	glGetFloatv(GL_VIEWPORT, viewport);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, viewport[2], 0.0, viewport[3], 1.0, -1.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glColor3f(1.0, 1.0, 1.0);
	
	glBlendFunc(GL_ONE, GL_ZERO);
	// restore what was there
	FMTexRectf(buffer->averageTex, w, h,
				0.0, 0.0, w, h);
	
	glDisable(GL_TEXTURE_RECTANGLE_EXT);
	
	// restore old state
	glPopAttrib();
}
