/*
 *  Texures.c
 *  FountainMusic
 *
 *  Copyright 2003, 2004, 2005, 2006 Brian Moore
 *
 */

#include "Textures.h"
#include "FMDefs.h"

float LinearFade(float p)
{
	if (p <= 0.0) return 1.0;
	else if (p >= 1.0) return 0.0;
	else return 1.0 - p;
}

#define GAUSS_WIDTH 0.3

float GaussianFade(float p)
{
	if (p <= 0.0) return 1.0;
	else if (p >= 1.0) return 0.0;
	else return expf(-(p*p)/(2*GAUSS_WIDTH*GAUSS_WIDTH));
}

float ThetaForVector(float x, float y)
{
	float len = hypot(x, y);
	float asi = asin(y/len);
	
	if (x > 0.0)
	{
		if (y > 0.0) return asi;
		else return 2.0*M_PI - asi;
	}
	else
	{
		if (y > 0.0) return M_PI - asi;
		else return M_PI + asi;
	}
}

#define MAX_LEVEL 8

void MakeAndPutRadialGradientForLevel(int level, float sharp)
{
    const int size = 1 << (MAX_LEVEL - level);
    GLubyte contextBuffer[size][size][4];
    int i, j;
    float dist, inRadius, outRadius;
	float theta;
	float fraction;
     
    for (i=0; i<size; i++)
    {
        for (j=0; j<size; j++)
        {
            dist = hypot(i-size/2.0, j-size/2.0);
			theta = ThetaForVector(i-size/2.0, j-size/2.0);
			
			outRadius = size/2.0 * 0.9;
			inRadius = outRadius*sharp;
            
			fraction = CLAMP((dist-inRadius)/(outRadius-inRadius), 0.0, 1.0);
			
            contextBuffer[i][j][0] = 
            contextBuffer[i][j][1] = 
            contextBuffer[i][j][2] = GaussianFade(fraction)*255.0;
            
            contextBuffer[i][j][3] = 255; 
        }
    }
    
    glTexImage2D(
            GL_TEXTURE_2D, 
            level, 
            GL_RGBA, 
            size,//CGImageGetWidth(texImage), 
            size,//CGImageGetHeight(texImage), 
            0, 
            GL_RGBA,
            GL_UNSIGNED_BYTE, 
            contextBuffer);
}

GLuint MakeRadialGradientTextureWithSharpness(float sharp)
{
    // make the GL texture now
    GLuint texName;
    int i;
        
    glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_2D, texName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
    for (i=0; i<=MAX_LEVEL; i++)
    {
        MakeAndPutRadialGradientForLevel(i, sharp);
    }
    
    return texName;
}

