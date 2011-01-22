/*
 *  Drawing.c
 *  FountainMusic
 *
 *  Copyright 2003, 2004, 2005, 2006, 2007 Brian Moore
 *
 */

//#define __DATA_DISP__

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#ifdef ALBUM_ART
#include <QuickTime/QuickTimeComponents.h>
#endif
#include "Drawing.h"

#define LEFT_MARGIN 15.0
#define BOTTOM_MARGIN 25.0
#define TOP_MARGIN 20.0
#define	ITEM_SPACING 5.0
#define ITEM_HEIGHT(h) ((h) + ITEM_SPACING)

#define kFMTempStringLength 128

OSStatus SetupAGL(VisualPluginData *myData )
{ 
    GLint bufferRect[4];
    Rect portRect;
    GLint attribs[] = {AGL_RGBA, GL_TRUE, 
                       AGL_ACCELERATED, GL_TRUE,
                       AGL_DOUBLEBUFFER, GL_TRUE,
                       AGL_DEPTH_SIZE, 24,
                       AGL_NONE};
    
	AGLPixelFormat fmt;
    
	// create a pixel format
    fmt = aglChoosePixelFormat(NULL, 0, attribs);
    if (fmt == NULL)
    {
		FDEBUG_OUT("Fountain Music Error: pixel format can not be made (%s)", aglErrorString(aglGetError()) );
		return notEnoughHardwareErr;
	}
	
	#ifdef FORCE_SOFTWARE_RENDER
	// this will force software rendering
	GLint idValue;
	AGLPixelFormat nextFmt;
	
	printf("Fountain Music Debug: forcing software renderer:\n");
	
	// while the current format is hardware accelerated
	while (PixelFormatAccelerated(fmt))
	{
		aglDescribePixelFormat(fmt, AGL_RENDERER_ID, &idValue);
		printf("Fountain Music Debug: discarding hardware renderer %i\n", (int)idValue);
		
		// get the next renderer
		nextFmt = aglNextPixelFormat(fmt);
		aglDestroyPixelFormat(fmt);
		fmt = nextFmt;
		
		// if there are no more renderers, refuse to continue
		if (fmt == NULL)
		{
			DEBUG_OUT("Fountain Music Error: software renderer could not be found!");
			return notEnoughHardwareErr;
		}
	}
	// by this point there should be a valid pixel format in fmt
	
	aglDescribePixelFormat(fmt, AGL_RENDERER_ID, &idValue);
	printf("Fountain Music Debug: using software renderer %i\n", (int)idValue);
	
	#endif
    
    // use our pixel format to create an OpenGL rendering context
    myData->glContext = aglCreateContext(fmt, NULL);
    
    if (myData->glContext == NULL) 
    {
        FDEBUG_OUT("Fountain Music Error: glContext can not be made (%s)", aglErrorString(aglGetError()) );
        return unimpErr;
    }
    
    // we no longer need the pixed format
    aglDestroyPixelFormat(fmt);
    
    // make our new context current
    aglSetCurrentContext(myData->glContext);

    if (!aglSetWindowRef(myData->glContext, GetWindowFromPort(myData->destPort)))
    {
        printf("Fountain Music Error: Could not set drawable for OpenGL context!\n");
        return unimpErr;
    }

    // take care of the bufferRect detail
    GetWindowPortBounds (GetWindowFromPort(myData->destPort), &portRect);
    bufferRect[0] = myData->destRect.left;
    bufferRect[1] = portRect.bottom - myData->destRect.bottom;
    bufferRect[2] = myData->destRect.right - myData->destRect.left;
    bufferRect[3] = myData->destRect.bottom - myData->destRect.top;
    
    aglSetInteger (myData->glContext, AGL_BUFFER_RECT, bufferRect);
    aglEnable (myData->glContext, AGL_BUFFER_RECT);

    
	// turn on VSync
	GLint one = 1;
	aglSetInteger(myData->glContext, AGL_SWAP_INTERVAL, &one);
	aglEnable(myData->glContext, AGL_SWAP_INTERVAL);
    
    aglUpdateContext(myData->glContext);
    
    return noErr;
}

void CleanupAGL(VisualPluginData *myData)
{
    // delete OpenGL context
    aglSetCurrentContext(NULL);
    aglDestroyContext(myData->glContext);
    myData->glContext = NULL;
}

void strncpyp2c(char *dest, StringPtr src, unsigned int destLen)
{
	int len = (destLen < src[0]) ? (destLen-1) : src[0];
	memcpy(dest, src+1, len);
	
	dest[len] = 0; // null terminate
}

const char *EdgeStringForSharpness(float sharp)
{ // 0.0 0.25 0.50 0.75 1.0
	if (sharp <= 0.125) return "Very Fuzzy";
	else if (sharp <= 0.375) return "Fuzzy";
	else if (sharp <= 0.625) return "Medium";
	else if (sharp <= 0.875) return "Sharp";
	else return "Very Sharp";
}

void InitDisplayItems(VisualPluginData *myData)
{
	myData->displayItems = FMNewList();
	
	myData->namedItems.fpsDispItem = FMDisplayItemCreate();
	myData->namedItems.particleCountDispItem = FMDisplayItemCreate();
	myData->namedItems.menuLine1DispItem = FMDisplayItemCreate();
	myData->namedItems.menuLine2DispItem = FMDisplayItemCreate();
	myData->namedItems.menuLine3DispItem = FMDisplayItemCreate();
	myData->namedItems.menuLine4DispItem = FMDisplayItemCreate();
	myData->namedItems.menuLine5DispItem = FMDisplayItemCreate();
	myData->namedItems.menuLine6DispItem = FMDisplayItemCreate();
	myData->namedItems.colorTableItem = FMDisplayItemCreate();
	myData->namedItems.modeDispItem = FMDisplayItemCreate();
	myData->namedItems.edgeLabelDispItem = FMDisplayItemCreate();
	myData->namedItems.titleDispItem = FMDisplayItemCreate();
	myData->namedItems.artistDispItem = FMDisplayItemCreate();
	myData->namedItems.albumDispItem = FMDisplayItemCreate();
	myData->namedItems.versDispItem = FMDisplayItemCreate();
	
	FMListPrependData(myData->displayItems, myData->namedItems.versDispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.fpsDispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.particleCountDispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.menuLine1DispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.menuLine2DispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.menuLine3DispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.menuLine4DispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.menuLine5DispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.menuLine6DispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.colorTableItem);
	FMListPrependData(myData->displayItems, myData->namedItems.modeDispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.edgeLabelDispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.titleDispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.artistDispItem);
	FMListPrependData(myData->displayItems, myData->namedItems.albumDispItem);

	// set fade times
	FMDisplayItemSetFadeTime(myData->namedItems.menuLine1DispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.menuLine2DispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.menuLine3DispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.menuLine4DispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.menuLine5DispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.menuLine6DispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.colorTableItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.modeDispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.edgeLabelDispItem, 0.5);
	
	FMDisplayItemSetFadeTime(myData->namedItems.titleDispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.artistDispItem, 1.0);
	FMDisplayItemSetFadeTime(myData->namedItems.albumDispItem, 1.0);
	
	FMDisplayItemSetFadeTime(myData->namedItems.versDispItem, 0.5);
	
	// set text sizes
	FMDisplayItemSetTextSize(myData->namedItems.titleDispItem, 13.0);
	FMDisplayItemSetTextSize(myData->namedItems.artistDispItem, 13.0);
	FMDisplayItemSetTextSize(myData->namedItems.albumDispItem, 13.0);
	FMDisplayItemSetTextSize(myData->namedItems.versDispItem, 12.0);
		
	// layout items:
	LayoutDisplayItems(myData);
	
	CFStringRef versionStr = GetCurrentVersion(myData);
	CFStringRef fmVersStr = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
										  CFSTR("Fountain Music %@"), 
										  versionStr);
										
	FMDisplayItemSetStringValueCFString(myData->namedItems.versDispItem, fmVersStr);
	
	CFRelease(fmVersStr);
}

void LayoutDisplayItems(VisualPluginData *myData)
{
	FMListEnumerator *itemEnumer;
	FMDisplayItemRef currItem;
	float currX, currY;
	
	itemEnumer = FMNewListEnumerator(myData->displayItems);
	
	currX = LEFT_MARGIN;
	currY = BOTTOM_MARGIN;
	
	while (currItem = FMListEnumeratorNextData(itemEnumer))
	{
		FMDisplayItemSetPosition(currItem, currX, currY);
		
		currY += ITEM_HEIGHT(FMDisplayItemGetTextSize(currItem));
	}
	
	FMDeleteListEnumerator(itemEnumer);
}

void UpdateAllDisplayItems(VisualPluginData *myData)
{
	char tempString[kFMTempStringLength];

	UpdateFPSDisplayItem(myData);
	UpdateParticleDisplayItem(myData);
	
	if (myData->showDebugMenu)
    {
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine1DispItem, CFSTR("~\tdisplay this menu"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine2DispItem, 
									myData->displayFPS ? CFSTR("f\thide FPS counter") : CFSTR("f\tshow FPS counter"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine3DispItem, 
									myData->displayNumParticles ? CFSTR("p\thide particle counter") : CFSTR("p\tshow particle counter"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine4DispItem, CFSTR("v\tdisplay version"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine5DispItem, CFSTR(""));
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine6DispItem, CFSTR(""));
	}
	else
	{
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine1DispItem, CFSTR("h ?\tdisplay this menu"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine2DispItem, CFSTR("i\tdisplay track info"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine3DispItem, CFSTR("- +\tdecrease/increase particle size"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine4DispItem, 
					FMFountainModeIsLocked(myData->modeData) ? CFSTR("[ ] \\\tprev/next/unlock pattern") : CFSTR("[ ] \\\tprev/next/lock pattern"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine5DispItem, CFSTR("; '\tprevious/next color"));
		
		FMDisplayItemSetStringValueCFString(myData->namedItems.menuLine6DispItem, CFSTR("< >\tfuzzier/crisper edges"));
	}
	
	CFStringRef tableName = CopyColorTableName(myData, myData->currTable);
	CFStringRef label = CFStringCreateWithFormat(NULL, NULL, CFSTR("Colors: %@"), tableName);
	
	FMDisplayItemSetStringValueCFString(myData->namedItems.colorTableItem, label);
	
	CFRelease(label);
	CFRelease(tableName);
	
	char modeName[128];
	FMFountainModeCopyModeName(myData->modeData, FMFountainModeGetCurrentMode(myData->modeData), modeName, 128);
	
	snprintf(tempString, kFMTempStringLength, "Pattern: %s%s", modeName, FMFountainModeIsLocked(myData->modeData) ? " (locked)" : "");
	FMDisplayItemSetStringValueMacRoman(myData->namedItems.modeDispItem, tempString);
	
	snprintf(tempString, kFMTempStringLength, "Edges: %s", EdgeStringForSharpness(myData->sharpness));
    FMDisplayItemSetStringValueMacRoman(myData->namedItems.edgeLabelDispItem, tempString);
	
	CFStringRef titleStr, artistStr, albumStr;
	
	titleStr = CFStringCreateWithCharacters(kCFAllocatorDefault, myData->currentSong.name+1, (CFIndex)(myData->currentSong.name[0]));
	artistStr = CFStringCreateWithCharacters(kCFAllocatorDefault, myData->currentSong.artist+1, (CFIndex)(myData->currentSong.artist[0]));
	albumStr = CFStringCreateWithCharacters(kCFAllocatorDefault, myData->currentSong.album+1, (CFIndex)(myData->currentSong.album[0]));
	
	FMDisplayItemSetStringValueCFString(myData->namedItems.titleDispItem, titleStr);
	FMDisplayItemSetStringValueCFString(myData->namedItems.artistDispItem, artistStr);
	FMDisplayItemSetStringValueCFString(myData->namedItems.albumDispItem, albumStr);
	
	CFRelease(titleStr);
	CFRelease(artistStr);
	CFRelease(albumStr);
}

void UpdateFPSDisplayItem(VisualPluginData *myData)
{
	CFStringRef fpsString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%.1f FPS"), myData->averageFPS);
		
	FMDisplayItemSetStringValueCFString(myData->namedItems.fpsDispItem, fpsString);
	
	CFRelease(fpsString);
}

void UpdateParticleDisplayItem(VisualPluginData *myData)
{
	CFStringRef particleString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%i Particle%s"), myData->supply->numberUsed,
																			(myData->supply->numberUsed == 1) ? "" : "s");

	FMDisplayItemSetStringValueCFString(myData->namedItems.particleCountDispItem, particleString);
	
	CFRelease(particleString);
}

void ShowFPSMeter(VisualPluginData *myData, boolean_t onFlag)
{	
	FMDisplayItemSetTimeToDie(myData->namedItems.fpsDispItem, onFlag?INFINITY:0.0);
	
	UpdateAllDisplayItems(myData);
}

void ShowParticleMeter(VisualPluginData *myData, boolean_t onFlag)
{
	FMDisplayItemSetTimeToDie(myData->namedItems.particleCountDispItem, onFlag?INFINITY:0.0);
	
	UpdateAllDisplayItems(myData);
}

void ShowMenu(VisualPluginData *myData)
{
	FMDisplayItemSetTimeToDie(myData->namedItems.menuLine1DispItem, 6.5); 
	FMDisplayItemSetTimeToDie(myData->namedItems.menuLine2DispItem, 6.2);
	FMDisplayItemSetTimeToDie(myData->namedItems.menuLine3DispItem, 5.9);
	FMDisplayItemSetTimeToDie(myData->namedItems.menuLine4DispItem, 5.6);
	FMDisplayItemSetTimeToDie(myData->namedItems.menuLine5DispItem, 5.3);
	FMDisplayItemSetTimeToDie(myData->namedItems.menuLine6DispItem, 5.0);
	
	UpdateAllDisplayItems(myData);
}

void ShowMode(VisualPluginData *myData)
{
	FMDisplayItemSetTimeToDie(myData->namedItems.modeDispItem, 3.0);
	
	UpdateAllDisplayItems(myData);
}

void ShowSharpnessLabel(VisualPluginData *myData)
{
	FMDisplayItemSetTimeToDie(myData->namedItems.edgeLabelDispItem, 2.0);
	
	UpdateAllDisplayItems(myData);
}

void ShowTrackInfo(VisualPluginData *myData, boolean_t always)
{
	FMDisplayItemSetTimeToDie(myData->namedItems.titleDispItem, always ? INFINITY : 4.8);
	FMDisplayItemSetTimeToDie(myData->namedItems.artistDispItem, always ? INFINITY : 4.4);
	FMDisplayItemSetTimeToDie(myData->namedItems.albumDispItem, always ? INFINITY : 4.0);
	
	UpdateAllDisplayItems(myData);
}

void ShowColorTable(VisualPluginData *myData)
{
	FMDisplayItemSetTimeToDie(myData->namedItems.colorTableItem, 3.0);
	
	UpdateAllDisplayItems(myData);
}

void ShowVersion(VisualPluginData *myData)
{
	FMDisplayItemSetTimeToDie(myData->namedItems.versDispItem, 7.0);
	
	UpdateAllDisplayItems(myData);
}

void StepDisplayItems(VisualPluginData *myData, float delta)
{
	FMListEnumerator *enumer = FMNewListEnumerator(myData->displayItems);
	FMListNode *currNode;
	
	FMDisplayItemRef currItem;
	
	while (currNode = FMListEnumeratorNext(enumer))
	{
		currItem = (FMDisplayItemRef)currNode->data;
		
		FMDisplayItemStep(currItem, delta);
	}
	
	FMDeleteListEnumerator(enumer);
}

void DrawOverlay(VisualPluginData *myData)
{
	FMListEnumerator *enumer = FMNewListEnumerator(myData->displayItems);
	FMListNode *currNode;
	
	FMDisplayItemRef currItem;
	
	while (currNode = FMListEnumeratorNext(enumer))
	{
		currItem = (FMDisplayItemRef)currNode->data;
		
		FMDisplayItemDraw(currItem);
	}
	
	FMDeleteListEnumerator(enumer);
}

void RecordRenderFrame(VisualPluginData *myData)
{
	float tim = TimeSinceStart(myData);
	int i;
	float avg = 0.0;
	float numSamples = FPS_AVG_SAMPLES;
	
	myData->renderTimes[myData->renderTimesHead] = tim - myData->lastRenderTime;
	myData->renderTimesHead = (myData->renderTimesHead+1)%FPS_AVG_SAMPLES;

	for (i=0; i < FPS_AVG_SAMPLES; i++) avg += myData->renderTimes[i];
	
	if (numSamples > 0.1 && avg > 0.00001)
	{
		avg /= numSamples;
		myData->averageFPS = 1.0/avg;
	}
	
	myData->lastRenderTime = tim;
}

void RenderScene(VisualPluginData *myData)
{
    if (myData->glContext && myData->isRendering == false)
    {        
        myData->isRendering = true;
        
		// Make 3D World:
		Setup3DSystem(myData);
		
		// Clean the slate
		glClearColor(0.0, 0.0, 0.0, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	
		// just to be sure my translations are correct
		glMatrixMode(GL_MODELVIEW);
		
		// reset and move fountain back, rotate it
		glLoadIdentity();
		glTranslatef(0.0, -3.0, -15.0);
		glRotatef(myData->angle, 0.0, 1.0, 0.0);
	
		// no depth testing
		glDisable( GL_DEPTH_TEST );
		glDepthFunc(GL_ALWAYS);
		glDepthMask(GL_FALSE);
		
		// neat blending for particles
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		
		// make sure I can draw the particle texture
		glEnable(GL_TEXTURE_2D);
		
		// bind the particle texture
		glBindTexture(GL_TEXTURE_2D, myData->texName);
		
		// activate vertex arrays
		static GLfloat squarevertices[] = 
		{
			-0.5, -0.5, 0.0,
			0.5, -0.5, 0.0,
			0.5, 0.5, 0.0,
			-0.5, 0.5, 0.0
		};
		static GLfloat squaretexs[] = 
		{
			0.0, 0.0,
			1.0, 0.0,
			1.0, 1.0,
			0.0, 1.0
		};
		
		static unsigned int indices[] = { 0, 1, 2, 3 };
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, squarevertices);
		glTexCoordPointer(2, GL_FLOAT, 0, squaretexs); 
		
		// enumerate and draw the particles
		FMPoint3D currPoint;
		FMListNode *currNode;
		Particle3D *currParticle;
		float currR, currG, currB;
		FMListEnumerator *usedParticleEnum;
		
		usedParticleEnum = FMNewListEnumerator(myData->supply->usedList);
		
		while (currNode = FMListEnumeratorNext(usedParticleEnum) )
		{
			currParticle = currNode->data;
			currPoint = currParticle->position;
			
			glPushMatrix();
			
				glTranslatef( currPoint.x, currPoint.y, currPoint.z );
				
				glScalef(myData->particleSize*currParticle->size,
						 myData->particleSize*currParticle->size,
						 myData->particleSize*currParticle->size);
						 
				// face it towards the camera
				glRotatef(-myData->angle, 0.0f, 1.0f, 0.0f);
				
				//glRotatef(currParticle->rotation * 180.0/M_PI, 0.0, 0.0, 1.0);
				
				GetParticleColor(currParticle, &currR, &currG, &currB);
				
				glColor3f(currR, currG, currB);
				
				//glCallList(myData->pList);
				glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);
						
			glPopMatrix();
		}
		
		FMDeleteListEnumerator(usedParticleEnum);
		
        #ifdef MOTION_BLUR
        FMGLMotionBufferAdd(myData->motionBuffer, CGPointZero); // record particles
        
		//glClearColor(0.0, 0.0, 0.0, 1.0); // erase that
		//glClear(GL_COLOR_BUFFER_BIT);
		
		FMGLMotionBufferAverage(myData->motionBuffer); // average the frames
		FMGLMotionBufferDraw(myData->motionBuffer, CGPointZero);
		#endif
    
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		
		// we are no longer using textures now
		glDisable( GL_TEXTURE_2D );
		
		// we want fading text to blend nicely
		glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
		glEnable(GL_BLEND);
		
		Setup2DSystem(myData);
		
		// info and menu
		DrawOverlay(myData);
		
		#ifdef DEBUG_DRAW_AUDIO
		
		float c;
		int i;

		glBegin(GL_LINES);
		for (i=0; i<kFMNumSpectrumEntries; i++)
		{
			c = (*FMAudioBufferGetAverage(myData->shortBuffer))[0][i];
			
			if (c - (*FMAudioBufferGetAverage(myData->minuteBuffer))[0][i] > 0.01) glColor3f(0.0, 1.0, 0.0);
			else glColor3f(1.0, 0.0, 0.0);
			
			glVertex2f(3*i, 300.0*c);
			glVertex2f(3*i, 0);
		}
		glEnd();  
		#endif

		#ifdef DEBUG_HISTORY
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_LINE_STRIP);
		for (i=0; i<HISTORY_LENGTH; i++)
		{
			glVertex2f(2*i, myData->particleHistory[(myData->historyHeadIdx+i)%HISTORY_LENGTH]);
		}
		glEnd();  
		#endif
			
		RecordRenderFrame(myData);
		UpdateFPSDisplayItem(myData);
        
		myData->isRendering = false;
    }
}

void SetupContextRect(VisualPluginData *myData)
{
    WindowRef window = GetWindowFromPort(myData->destPort);
    GLint bufferRect[4];
    Rect portRect;
    
    aglSetWindowRef(myData->glContext, window);
    
    GetWindowPortBounds (window, &portRect);
    bufferRect[0] = myData->destRect.left;
    bufferRect[1] = portRect.bottom - myData->destRect.bottom;
    bufferRect[2] = myData->destRect.right - myData->destRect.left;
    bufferRect[3] = myData->destRect.bottom - myData->destRect.top;
    

    aglSetInteger (myData->glContext, AGL_BUFFER_RECT, bufferRect);
    aglEnable (myData->glContext, AGL_BUFFER_RECT);
    
    aglUpdateContext(myData->glContext);
}

void Setup3DSystem(VisualPluginData *myData)
{
    GLint bufferRect[4];
    
    Rect portRect;
    // **WINDOWS_DIFF** is this tomfoolery neccesary on doze?
    GetWindowPortBounds (GetWindowFromPort(myData->destPort), &portRect);
    bufferRect[0] = myData->destRect.left;
    bufferRect[1] = portRect.bottom - myData->destRect.bottom;
    bufferRect[2] = myData->destRect.right - myData->destRect.left;
    bufferRect[3] = myData->destRect.bottom - myData->destRect.top;
    
    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();
    
    glViewport(0, 0, bufferRect[2], bufferRect[3]);
    
    if (bufferRect[2] < bufferRect[3]) gluPerspective(myData->fov, ((float)bufferRect[2])/((float)bufferRect[3]), 0.01f, 100.0f);
    else gluPerspective(myData->fov, ((float)bufferRect[2])/((float)bufferRect[3]), 0.01f, 100.0f);
    
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
}

void Setup2DSystem(VisualPluginData *myData)
{

    GLint bufferRect[4];
    
    Rect portRect;
    GetWindowPortBounds (GetWindowFromPort(myData->destPort), &portRect);
    
    bufferRect[0] = myData->destRect.left;
    bufferRect[1] = portRect.bottom - myData->destRect.bottom;
    bufferRect[2] = myData->destRect.right - myData->destRect.left;
    bufferRect[3] = myData->destRect.bottom - myData->destRect.top;
    
    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();
    
    glViewport(0, 0, bufferRect[2], bufferRect[3]);
    
    glOrtho(0.0, (float)bufferRect[2], 0.0, (float)bufferRect[3], 1.0, -1.0);
    
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
}

#ifdef ALBUM_ART
void ClearAlbumArt(VisualPluginData *myData) {
	
}

void SetAlbumArt(VisualPluginData *myData, Handle artHandle, OSType artType)
{
	OSErr err = noErr;
	
	ComponentInstance importer = NULL;

	// get the graphics importer
	err = GetGraphicsImporterForDataRef(artHandle, artType, &importer);
	if (err != noErr) {
		fprintf(stderr, "Fountain Music Error: Could not get graphics importer (err %i)\n", err);
	}
	
	// close it up!
	CloseComponent(importer);
}

#endif

