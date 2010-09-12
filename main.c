/*
 *  main.c
 *  main entry point for Fountain Music
 *
 *  Copyright 2003, 2004, 2005, 2006, 2007 Brian Moore
 *
 */

#include "iTunesVisualAPI.h"

#include <OpenGL/gl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "FMDefs.h"
#include "FountainModes.h"
#include "ParticleEngine.h"
#include "Drawing.h"
#include "FMConfigure.h"
#include "FMPrefs.h"

#define IMPEXP
#define	MAIN iTunesPluginMainMachO

static OSStatus VisualPluginHandler(OSType message, VisualPluginMessageInfo *messageInfo,void *refCon);
IMPEXP OSStatus MAIN(OSType message, PluginMessageInfo *messageInfo, void *refCon);

OSStatus VPHandleRenderMessage(VisualPluginRenderMessage *renderMessage, VisualPluginData *myData);
OSStatus VPHandleEventMessage(VisualPluginEventMessage *eventMessage, VisualPluginData *myData);
OSStatus VPHandleShowWindowMessage(VisualPluginShowWindowMessage *showWindowMessage, VisualPluginData *myData);

void FMProcessAudioData(VisualPluginRenderMessage *renderMessage, VisualPluginData *myData);
void FMStepAnimations(VisualPluginData *myData);
void FMRender(VisualPluginData *myData);

void StartRenderTimer(VisualPluginData *myData);
void StopRenderTimer(VisualPluginData *myData);

Boolean PixelFormatAccelerated(AGLPixelFormat fmt);

static OSStatus VisualPluginHandler(OSType message, VisualPluginMessageInfo *messageInfo,void *refCon)
{
    OSStatus status = noErr;
    
    VisualPluginData *myData = refCon;
    
    #ifdef DEBUG_MESSAGES    
    
    #ifdef DEBUG_MESSAGES_NOIDLE
    if (message != kVisualPluginIdleMessage)
    {
    #endif
    
    printf("Fountain Music Debug: visual handler message: %c%c%c%c\n", 
                (char) (message >> 24), (char) (message >> 16), (char) (message >> 8), (char) message);
    
    #ifdef DEBUG_MESSAGES_NOIDLE
    }
    #endif
    
    #endif
    
    
    switch (message)
    {
		#pragma mark kVisualPluginInitMessage
		case kVisualPluginInitMessage: // on startup, first message plugin receives
		
            messageInfo->u.initMessage.refCon = refCon;
            messageInfo->u.initMessage.options = 0;
			
			myData->appCookie = messageInfo->u.initMessage.appCookie;
			myData->appProc = messageInfo->u.initMessage.appProc;
			
			InitMyDataValues(myData);
			FMRestoreAllFromPrefs(myData);
            
			break;
			
		#pragma mark kVisualPluginCleanupMessage
		case kVisualPluginCleanupMessage: // iTunes about to quit
		
			FMSaveAllToPrefs(myData);
			
			break;
	
        #pragma mark kVisualPluginShowWindowMessage
        case kVisualPluginShowWindowMessage: // activate visual
            
			status = VPHandleShowWindowMessage(&(messageInfo->u.showWindowMessage), myData);
            break;
            
        #pragma mark kVisualPluginHideWindowMessage
        case kVisualPluginHideWindowMessage: // deactivate visual

			
            myData->isActivated = false;
        
			// delete OpenGL textures
            glDeleteTextures(1, &(myData->texName));
            myData->texName = 0;
            
            glDeleteLists(myData->pList, 1);
            myData->pList = 0;
            
            myData->fontLists = 0;
        
            CleanupAGL(myData);
            
			#ifdef MOTION_BLUR
			FMGLMotionBufferDelete(myData->motionBuffer);
			myData->motionBuffer = FALSE;
			#endif
			
			// invalidate render timer
			StopRenderTimer(myData);
			
            break;
            
        #pragma mark kVisualPluginRenderMessage
        case kVisualPluginRenderMessage:
			status = VPHandleRenderMessage(&(messageInfo->u.renderMessage), myData);
            break;
            

        
        #pragma mark kVisualPluginIdleMessage
        case kVisualPluginIdleMessage:
			// Nothing to do. Idle messages are silly anyway, don't use them.
            break;
			
			
		#pragma mark kVisualPluginEventMessage
		case kVisualPluginEventMessage:
			status = VPHandleEventMessage(&(messageInfo->u.eventMessage), myData);
			break;
            
		#pragma mark kVisualPluginPlayMessage
		case kVisualPluginPlayMessage:
		{
			Boolean isChangeTrack;
			UInt32 oldTime = myData->currentSong.totalTimeInMS;
			
			myData->isPlaying = true;
			
			// set current song info - this is okay because we're assigning structs, NOT pointers, will copy
			myData->currentSong = *(messageInfo->u.playMessage.trackInfoUnicode);
			
			#ifdef ALBUM_ART
			Handle artHandle;
			OSType artType;
			
			printf("getting album art...\n");
			PlayerGetCurrentTrackCoverArt(myData->appCookie, myData->appProc, &artHandle, &artType);
			printf("got album art! type:%.4s\n", (char *)(&artType));
			
			// must open and use a GraphicsImporter component to decode image,
			// then transfer into a texture...
			if (artType != NULL) CopyAlbumArtToTexture(myData, artHandle, artType);
			
			#endif
			
			isChangeTrack = (myData->currentSong.totalTimeInMS != oldTime) && (oldTime != 0);
			
			if (myData->isActivated)
			{
				myData->lastStepTime = CURR_TIME;
				
				UpdateAllDisplayItems(myData);
				ShowTrackInfo(myData, myData->alwaysShowTrackInfo);
			}
			else // clear old particles if we've changed to a new song 
			{
				if (isChangeTrack)
				{
					ResetParticleSupply(myData->supply);
					FMAudioBufferZero(myData->minuteBuffer);
					FMAudioBufferZero(myData->shortBuffer);
				}
			}
		}	
			break;
		

		#pragma mark kVisualPluginStopMessage
		case kVisualPluginStopMessage: // music stopped, find a chair!
		
			myData->isPlaying = false;
	
			break;
			
		#pragma mark kVisualPluginConfigureMessage
		case kVisualPluginConfigureMessage:
			status = FMRunConfig(myData);
			break;
			
		#pragma mark kVisualPluginSetWindowMessage
		case kVisualPluginSetWindowMessage:
				
			if (myData->isActivated)
			{
				myData->destRect = messageInfo->u.setWindowMessage.drawRect;
				myData->destPort = messageInfo->u.setWindowMessage.port;
				
				SetupContextRect(myData);
			}
			
			break;
		
		#pragma mark kVisualPluginChangeTrackMessage
		case kVisualPluginChangeTrackMessage: // as in track info changed
		
			myData->currentSong = *(messageInfo->u.changeTrackMessage.trackInfoUnicode);
		
			if (myData->isActivated)
			{
				UpdateAllDisplayItems(myData);
				ShowTrackInfo(myData, myData->alwaysShowTrackInfo);
			}
			break;
			
		#pragma mark kVisualPluginUpdateMessage
		case kVisualPluginUpdateMessage:
				
			if (myData->isActivated)
			{
				RenderScene(myData);
				
				FMSwapBuffers(myData);
			}
			
			break;
			
		#pragma mark Ignored Messages
		case kVisualPluginEnableMessage:  // iTunes supports these, but I don't use them
		case kVisualPluginDisableMessage:
		case kVisualPluginSetPositionMessage:
			status = noErr; // returning unimp breaks plugin
			break;
		
		#pragma mark Unsupported Messages
		case kVisualPluginPauseMessage: // iTunes doesn't use these SoundJam relics
		case kVisualPluginUnpauseMessage:
			status = unimpErr;
			break;
			
    }
    
    return status;
}

IMPEXP OSStatus MAIN(OSType message, PluginMessageInfo *messageInfo, void *refCon)
{
    #ifdef DEBUG_MESSAGES    
    
    #ifdef DEBUG_MESSAGES_NOIDLE
    if (message != kPlayerIdleMessage)
    {
    #endif
    
    printf("Fountain Music Debug: main message: %c%c%c%c\n", 
                (char) (message >> 24), (char) (message >> 16), (char) (message >> 8), (char) message);
    
    #ifdef DEBUG_MESSAGES_NOIDLE
    }
    #endif
    
    #endif
    
    OSStatus status = noErr;
	
	// why not just do this right off the bat
	srand(time(NULL));
    
    switch (message)
    {
		#pragma mark kPluginInitMessage
        case kPluginInitMessage:
            ;
            VisualPluginData *myData = calloc( sizeof(VisualPluginData), 1 );
            
            if (!myData)
            {
                DEBUG_OUT("Fountain Music Error: myData could not be allocated in main!");
                return memFullErr;
            }
            
            myData->appCookie = messageInfo->u.initMessage.appCookie;
            myData->appProc = messageInfo->u.initMessage.appProc;
            myData->startupTime = CurrDoubleTime();
            
            messageInfo->u.initMessage.options = 0;
            messageInfo->u.initMessage.refCon = myData;
			
			//
			// Fill out registration structure
			//
            PlayerMessageInfo registerMessage;
            
            memset(&registerMessage, 0, sizeof(PlayerMessageInfo));
            
            Str63 nam = "\pFountain Music";
            memcpy( registerMessage.u.registerVisualPluginMessage.name, nam, nam[0]+1 );
            
			registerMessage.u.registerVisualPluginMessage.options = kVisualWantsConfigure;
            registerMessage.u.registerVisualPluginMessage.creator = 'hook';
            
			SetNumVersion(&(myData->version), 0x0, 0x0, finalStage, 0);
			registerMessage.u.registerVisualPluginMessage.pluginVersion = myData->version;

            registerMessage.u.registerVisualPluginMessage.handler = (VisualPluginProcPtr)VisualPluginHandler;
            registerMessage.u.registerVisualPluginMessage.registerRefCon = myData;
            
            registerMessage.u.registerVisualPluginMessage.timeBetweenDataInMS = 0xFFFFFFFF;
            registerMessage.u.registerVisualPluginMessage.numWaveformChannels = 0;
            registerMessage.u.registerVisualPluginMessage.numSpectrumChannels = 2;
            
            registerMessage.u.registerVisualPluginMessage.minWidth = 64;
            registerMessage.u.registerVisualPluginMessage.minHeight = 64;
            
            registerMessage.u.registerVisualPluginMessage.maxWidth = 32767;
            registerMessage.u.registerVisualPluginMessage.maxHeight = 32767;
			
            registerMessage.u.registerVisualPluginMessage.minFullScreenBitDepth = 0;
            registerMessage.u.registerVisualPluginMessage.maxFullScreenBitDepth = 0;
			registerMessage.u.registerVisualPluginMessage.windowAlignmentInBytes = 0;
			
			status = PlayerRegisterVisualPlugin(messageInfo->u.initMessage.appCookie, messageInfo->u.initMessage.appProc, &registerMessage);
			
			if (status != noErr) printf("Fountain Music Error: plugin registration failed with error code %i!\n", (int)status);
			
			break;
        
		#pragma mark kPluginCleanupMessage
        case kPluginCleanupMessage:
            free(refCon);
            
            status = noErr;
            break;
			
        #pragma mark kPluginIdleMessage
        case kPluginIdleMessage:
            status = unimpErr;
            break;
    }
    
    return status;
    
}

OSStatus VPHandleShowWindowMessage(VisualPluginShowWindowMessage *showWindowMessage, VisualPluginData *myData)
{
	OSStatus status = noErr;
    
	// just reaffirming that we ARE NOT activated yet (not set up at least)
    // we will know we are for sure when we get a render message
    // apparently iTunes sends idle messages asynchronously during other stuff,
    // so setting this to true makes it try to render, eek
    myData->isActivated = false;
	
	// setup drawing API
    myData->destPort = showWindowMessage->port;
    myData->destRect = showWindowMessage->drawRect;
    myData->destOptions = showWindowMessage->options;       
    
    // setup our AGL rendering context
	status = SetupAGL(myData);
	
    if (status != noErr)
    {
        printf("Fountain Music Error: The rendering context couldn't be setup for some reason.\n");
        return status;
    }
    
	#pragma mark -- Create Particle Display List 
	myData->pList = glGenLists(1);

	glNewList(myData->pList, GL_COMPILE);
	
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-0.5, -0.5, 0.0);
		
		glTexCoord2f(1.0, 0.0);
		glVertex3f(0.5, -0.5, 0.0);
		
		glTexCoord2f(1.0, 1.0);
		glVertex3f(0.5, 0.5, 0.0);
		
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-0.5, 0.5, 0.0);
	glEnd();
	
	glEndList();

	#pragma mark -- Other setup
        
    myData->lastStepTime = CURR_TIME;
	myData->lastRenderTime = CURR_TIME;
	
	#ifdef MOTION_BLUR
	myData->motionBuffer = FMGLMotionNewBuffer(5, CGSizeMake(bufferRect[2], bufferRect[3]));
	#endif
	
	InitializeTextures(myData);
	
	// now we're all set up
	myData->isActivated = true;

	ShowTrackInfo(myData, myData->alwaysShowTrackInfo); // a bit of higher level scripting
	
	// finally, set up a CFRunLoop timer for rendering
	StartRenderTimer(myData);
	
	return status;
}

void FMProcessRenderData(VisualPluginData *myData, RenderVisualData *renderData) {
#pragma mark -- Averaging Stuff
	
	// add now sample data to buffers
	FMAudioBufferInsertITSample(myData->minuteBuffer, renderData->spectrumData);
	FMAudioBufferInsertITSample(myData->shortBuffer, renderData->spectrumData);
	
	// compute the averages
	FMAudioBufferComputeAverage(myData->minuteBuffer);
	FMAudioBufferComputeAverage(myData->shortBuffer);	
	
	// compute a delta
	FMAudioSubtractSpectrumData(FMAudioBufferGetAverage(myData->shortBuffer),
								FMAudioBufferGetAverage(myData->minuteBuffer), 
								&myData->relativePercent);
	
	Particle3D *particle;
	float numNewParticlesF;
	int numNewParticlesI;
	int j, chanNum;
	
	// just a bit of bookkeeping, this may be out of place
	myData->births = 0;
	myData->deaths = 0;
	
	int i;
	// particle creation loop, for every meaningful sample in our processed data
	for (i=0; i<kFMNumSpectrumEntries; i++)
	{
		chanNum = rand()%2;
		
		// determine the exact number of new particles to create
		// make float value, using remainder from last time
		numNewParticlesF = (myData->relativePercent[chanNum][i] > 0.007) * 1.00 +
		myData->particleNumRemainder;
		
		// store remainder from this time
		myData->particleNumRemainder = numNewParticlesF - floorf(numNewParticlesF);
		
		// round down to integer
		numNewParticlesI = (int)floorf(numNewParticlesF);
		
		// create the new particles
		for (j=0; j<numNewParticlesI; j++)
		{
			// get a free particle
			particle = GetParticle(myData->supply);
			
			if (particle) // we may have run out of particles
			{
				myData->births++; // bookkeeping
				
				// time to die is constant now, only a precaution against immortal particles
				particle->timeToDie = 10.0;
				
				// this call sets the location, velocity, acceleration, etc of 
				// the particle given it's representative frequency/amplitude
				FMFountainModeSetupParticle(myData->modeData, particle, i/255.0f, myData->relativePercent[chanNum][i], chanNum);
				
				particle->accel.y = myData->currGravity;
				
				// set its color according to the current color table
				particle->color.r = ColorTableComponentValue(myData, i/255.0, 0)
				*myData->relativePercent[chanNum][i]*3.0;
				
				particle->color.g = ColorTableComponentValue(myData, i/255.0, 1)
				*myData->relativePercent[chanNum][i]*3.0;
				
				particle->color.b = ColorTableComponentValue(myData, i/255.0, 2)
				*myData->relativePercent[chanNum][i]*3.0;
			}
			//else DEBUG_OUT("out of particles!\n");
		}                
	}
	
#ifdef DEBUG_HISTORY
	myData->particleHistory[myData->historyHeadIdx] = myData->births;
	myData->historyHeadIdx = (myData->historyHeadIdx+1)%HISTORY_LENGTH;
#endif
	
}

void FMStepAnimations(VisualPluginData *myData) {
	// compute time step for integration
	float stepTime = CURR_TIME-(myData->lastStepTime);
	
	if (myData->isPlaying) {
		// step the mode's animation
		FMFountainModeStepAnimation(myData->modeData, stepTime);
		
		// step along the color table in time
		myData->currTableColumn += stepTime*myData->currTableSpeed;
		
		// clamp the color table to a safe range
		while ( (int)myData->currTableColumn >= myData->tableWidth ) myData->currTableColumn -= myData->tableWidth;
		
		// move every particle
		StepSupply(myData->supply, stepTime);
		
		// reset particles out of bounds
		myData->deaths = HandleSupplyOB(myData->supply);
		UpdateParticleDisplayItem(myData);
	}
	
	// rotate display
#ifndef NO_SPIN
	myData->angle -= stepTime*20.0;
	myData->angle = ClampAngle(myData->angle); // (keep it within 0..2pi) 
#endif
	
	// fade screen items out
	StepDisplayItems(myData, stepTime);
	
	// set the new render time
	myData->lastStepTime = CURR_TIME;
}

void FMRender(VisualPluginData *myData) {
	aglSetCurrentContext(myData->glContext);
	
	RenderScene(myData);
	FMSwapBuffers(myData);
}

OSStatus VPHandleRenderMessage(VisualPluginRenderMessage *renderMessage, VisualPluginData *myData)
{
	OSStatus status = noErr;
	if (myData->isActivated)
    {
		// just process the data, don't even render (that's the render timer's responsibility)
        FMProcessRenderData(myData, renderMessage->renderData);
	}
	
	return status;
}

OSStatus VPHandleEventMessage(VisualPluginEventMessage *eventMessage, VisualPluginData *myData)
{
	OSStatus status;
	status = noErr;
	
	if (myData->isActivated == FALSE) return status;
	
	EventRecord* tEventPtr = eventMessage->event;
	
	if ((tEventPtr->what == keyDown) || (tEventPtr->what == autoKey))
	{    // charCodeMask,keyCodeMask;
		char theChar = tEventPtr->message & charCodeMask;
		
		switch (theChar)
		{
		   
			case ',':
			case '<':
				
			
				SetParticleSharpness(myData, floor(myData->sharpness*4.0)/4.0-0.25);
				FMConfigSyncUI(myData);
				
				ShowSharpnessLabel(myData);
				break;
			
			case '.':
			case '>':
				SetParticleSharpness(myData, floor(myData->sharpness*4.0)/4.0+0.25);
				FMConfigSyncUI(myData);
				
				ShowSharpnessLabel(myData);
				break;
			
			case ']':
									
				FMFountainModeIncrement(myData->modeData);
				
					
				ShowMode(myData);
					
				break;
				
			case '[':
		
				FMFountainModeDecrement(myData->modeData);
		
				ShowMode(myData);
				
				break;
				
			case '\\':
		
				FMFountainModeSetLocked(myData->modeData, !FMFountainModeIsLocked(myData->modeData));
		
				ShowMode(myData);
				
				break;
			
			case 'f':
			case 'F':
				SetShowsFPSCounter(myData, !myData->displayFPS);
				ShowFPSMeter(myData, myData->displayFPS);
				FMConfigSyncUI(myData);
				break;
				
			case 'P':
			case 'p':
				SetShowsParticleCounter(myData, !myData->displayNumParticles);
				ShowParticleMeter(myData, myData->displayNumParticles);
				FMConfigSyncUI(myData);
				break;
				
			
			case 'i':
			case 'I':
				
				ShowTrackInfo(myData, myData->alwaysShowTrackInfo);
				
				break;
				
			case '~':
			case '`':  
			
				myData->showDebugMenu = true;
				
				ShowMenu(myData);
				
				break;
				
			case '?':
			case '/':
			case 'h':
			case 'H':    
			
				myData->showDebugMenu = false;
				
				ShowMenu(myData);
				
				break;
				
			case '+':
			case '=':
			
				SetParticleSize(myData, myData->particleSize+0.04);
				FMConfigSyncUI(myData);
				break;
				
										
			case '-':
			case '_':
			
				SetParticleSize(myData, myData->particleSize-0.04);
				FMConfigSyncUI(myData);
				break;
				
			case 'v':
			case 'V':
				ShowVersion(myData);
				break;
				
			case ';':
			case ':':
                if (CFArrayGetCount(myData->tableTable) > 1)
                {
                    SetColorTableIndex(myData, (CFArrayGetCount(myData->tableTable)+myData->currTable-1)%CFArrayGetCount(myData->tableTable));
                    FMConfigSyncUI(myData);
                }
                
                ShowColorTable(myData);
				break;
			
			case '\'':
			case '"':
                if (CFArrayGetCount(myData->tableTable) > 1)
                {
                    SetColorTableIndex(myData, (myData->currTable+1)%CFArrayGetCount(myData->tableTable));
                    FMConfigSyncUI(myData);
                }
                
                ShowColorTable(myData);
				break;
			
			default:
				status = unimpErr;
		}
	}
	else
	{
		status = unimpErr;
	}
	
	return status;
}

Boolean PixelFormatAccelerated(AGLPixelFormat fmt)
{
	GLint flag;
	
	aglDescribePixelFormat(fmt, AGL_ACCELERATED, &flag);
	
	return flag == GL_TRUE;
}

#pragma mark -
#pragma mark Render Timer
void RenderTimerFired(CFRunLoopTimerRef timer, void *info) {
	VisualPluginData *myData = info;
	
	if (myData->isActivated)
	{
		FMStepAnimations(myData);
		FMRender(myData);
	}
	
}

void StartRenderTimer(VisualPluginData *myData) {
	CFRunLoopTimerContext context = {0, myData, NULL, NULL, NULL};	
	CFRunLoopTimerRef newTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, 0, 1.0/60.0, 0, 0, RenderTimerFired, &context);
	
	myData->renderTimer = newTimer;
	
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), newTimer, kCFRunLoopCommonModes);
	
}

void StopRenderTimer(VisualPluginData *myData) {
	
	if (myData->renderTimer) {
		CFRunLoopTimerInvalidate(myData->renderTimer);
		CFRelease(myData->renderTimer);
		myData->renderTimer = NULL;
	}
}


