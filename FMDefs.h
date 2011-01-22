/*
 *  FMDefs.h
 *  FountainMusic
 *
 *  Copyright 2003-2011 Brian Moore
 *
 */

#ifndef _FMDEFS_H
#define _FMDEFS_H

//
///// EXPERIMENTAL AND DEBUG FLAGS
//
//#define DEBUG_DRAW_AUDIO

//#define DEBUG_HISTORY
#ifdef DEBUG_HISTORY
#define HISTORY_LENGTH 200
#endif

//#define MOTION_BLUR

#ifdef MOTION_BLUR
	// what effect blur frames have
	#define BLUR_FRAMES 2
#endif

//#define DEBUG_MESSAGES
//#define DEBUG_MESSAGES_NOIDLE

//#define NO_SPIN
//#define FORCE_SOFTWARE_RENDER
//#define ALBUM_ART

//
///// END FLAGS
//

//
///// Includes
//
#include "iTunesVisualAPI.h"
#include "ParticleEngine.h"
#include "FountainModes.h"
#include "FMAudioProcessing.h"
#include "FMDisplayItem.h"

#ifdef MOTION_BLUR
#include "FMGLMotion.h"
#endif

#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>
#include <AGL/agl.h>
#include <OpenGL/gl.h>

//
///// Global Declarations
//
#define FDEBUG_OUT(message, a) printf(message"\n", a)
#define DEBUG_OUT(message) printf(message"\n")

#define CURR_TIME TimeSinceStart(myData)


#define FPS_AVG_SAMPLES	15

#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)
#define CLAMP(x,a,b) MIN(MAX(x,a),b)

typedef struct _FMConfigureSettings
{
	float particleSharpness;
	float particleSize;
	float gravityMultiplier;
	Boolean displayFPS;
	Boolean displayNumParticles;
	Boolean constantTI;
	int colorIdx;
	Float32 textSize;
} FMConfigureSettings;


typedef struct VisualPluginData {
	// iTunes Bookkeeping
	void *				appCookie;
	ITAppProcPtr		appProc;
	
	NumVersion version;
	
	// basic plugin state flags
	Boolean isPlaying;
	Boolean isActivated;
	Boolean isRendering;

	// iTunes track info
	ITTrackInfo currentSong;
	
	// drawing data
	CGrafPtr			destPort;
	    
	Rect				destRect;
	OptionBits			destOptions;
    
	// render timer
	CFRunLoopTimerRef renderTimer;
	
	// OpenGL stuff
	AGLContext glContext;
	
	GLuint pList;	// particle display list
	GLuint texName; // particle texture
	GLuint fontLists;

	// misc
	double startupTime;
	float lastStepTime;
	float particleNumRemainder;
	float angle;
	ParticleSupply *supply;
	Float32 textSize;
	float fov;	

	// user settable parameters
	float particleSize;
	float sharpness;
	float currGravity;
	
	// FPS data
	float lastRenderTime;
	int renderTimesHead;
	float renderTimes[FPS_AVG_SAMPLES];
	float averageFPS;

	// Color table data
	int tableWidth;
	int tableHeight;
	UInt8 *rgbColorTable;
	int currTable;
	float currTableSpeed;
	float currTableColumn;
	CFArrayRef tableTable;
	CFURLRef tablesPath;
	
	// display flags
	Boolean showDebugMenu;
	Boolean displayFPS;
	Boolean displayNumParticles;
	Boolean alwaysShowTrackInfo;
	
	// fountain mode data
	FMFountainModeDataRef modeData;
	
	// audio processing data
	FMAudioBufferRef shortBuffer;
	FMAudioBufferRef minuteBuffer;
	FMAudioSpectrumData relativePercent;
	
	// configuration dialog stuff
	Boolean configWindowRunning;
	FMConfigureSettings oldSettings;
	WindowRef configWindow;
	ControlRef gravitySlider;
	ControlRef pSharpnessSlider;
	ControlRef pSizeSlider;
	ControlRef fpsCheckBox;
	ControlRef pCountCheckBox;
	ControlRef constantTICheckBox;
	ControlRef colorPopup;
	ControlRef textSizePopup;
	
	// Particle Census
	int births;
	int deaths;
	
	// display objects
	FMList *displayItems;
	struct {
		FMDisplayItemRef fpsDispItem;
		FMDisplayItemRef particleCountDispItem;
		
		FMDisplayItemRef menuLine1DispItem;
		FMDisplayItemRef menuLine2DispItem;
		FMDisplayItemRef menuLine3DispItem;
		FMDisplayItemRef menuLine4DispItem;
		FMDisplayItemRef menuLine5DispItem;
		FMDisplayItemRef menuLine6DispItem;
		
		FMDisplayItemRef colorTableItem;
		FMDisplayItemRef modeDispItem;
		FMDisplayItemRef edgeLabelDispItem;
		
		FMDisplayItemRef titleDispItem;
		FMDisplayItemRef artistDispItem;
		FMDisplayItemRef albumDispItem;
		
		FMDisplayItemRef versDispItem;
	} namedItems;
	
	#ifdef DEBUG_HISTORY
	short particleHistory[HISTORY_LENGTH];
	short historyHeadIdx;
	#endif
	
	#ifdef MOTION_BLUR
	FMGLMotionBufferRef motionBuffer;
	#endif
	
} VisualPluginData;

// misc
void InitMyDataValues(VisualPluginData *myData);
//void InitActiveMyDataValues(VisualPluginData *myData);
//void CleanupActiveMyDataValues(VisualPluginData *myData);
void InitializeTextures();
inline void FMSwapBuffers(VisualPluginData *myData);


// utility methods
double CurrDoubleTime();
float RandomFloatBetween(float, float);
float ClampAngle(float angle);
float TimeSinceStart(VisualPluginData *myData);
float FMGaussianProb(float mean, float stdDev, float value);

// color table API
void InitColorTables(VisualPluginData *myData);
void InitFallbackColorTables(VisualPluginData *myData);
void SetColorTableIndex(VisualPluginData *myData, int newIndex);
int FindColorTableByName(VisualPluginData *myData, CFStringRef inName);
CFStringRef CopyColorTableName(VisualPluginData *myData, int idx);
CFURLRef CopyColorTablePath(VisualPluginData *myData, int idx);
float ColorTableComponentValue(VisualPluginData *myData, float frequencyPercent, int component);

// accessor functions

float GetParticleSharpness(VisualPluginData *myData);
void SetParticleSharpness(VisualPluginData *myData, float newSharpness);

float GetGravityMultiplier(VisualPluginData *myData);
void SetGravityMultiplier(VisualPluginData *myData, float mult);

float GetParticleSize(VisualPluginData *myData);
void SetParticleSize(VisualPluginData *myData, float newSize);

void SetShowsFPSCounter(VisualPluginData *myData, Boolean doesShow);
void SetShowsParticleCounter(VisualPluginData *myData, Boolean doesShow);

Boolean GetConstantTrackInfo(VisualPluginData *myData);
void SetConstantTrackInfo(VisualPluginData *myData, Boolean isConstant);

Float32 GetTextSize(VisualPluginData *myData);
void SetTextSize(VisualPluginData *myData, Float32 inSize);

CFStringRef GetCurrentVersion(VisualPluginData *myData);
#endif
