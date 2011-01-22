/*
 *  FMDefs.c
 *  FountainMusic
 *
 *  Copyright 2003-2011 Brian Moore
 *
 */
 
#pragma mark Declarations/Prototypes

#include "FMDefs.h"
#include "FountainModes.h"
#include "Textures.h"
#include "FMPrefs.h"
#include "Drawing.h"
#include "hsbrgb.h"

#define GRAV (-10.0)
#define FMImageIndex(x, y, c, width, height, comps) (comps*(y*width + x) + c)

void MakeColorTableFromPNG(VisualPluginData *myData, CFURLRef fileURL);
CFComparisonResult FMSortDisplayMode(void *val1, void *val2, void *context);


#pragma mark -
#pragma mark Misc

void InitMyDataValues(VisualPluginData *myData)
{
	int i;
	
	myData->isPlaying = FALSE;
    myData->isActivated = FALSE;
	myData->isRendering = FALSE;
	
	memset(&myData->currentSong, 0, sizeof(ITTrackInfo));

    myData->destPort = NULL;
    
    myData->glContext = NULL;
    
	myData->pList = 0;
	myData->texName = 0;
	myData->fontLists = 0;
	myData->textSize = 13.0;
	
	myData->lastStepTime = 0.0f;
	myData->particleNumRemainder = 0.0;
    myData->angle = RandomFloatBetween(0, 2.0*M_PI);
	myData->supply = NewParticleSupply();
	
	myData->fov = 45.0;
	
	myData->particleSize = 0.2;
	myData->sharpness = 0.0f;
	myData->currGravity = GRAV;
			
	myData->lastRenderTime = 0.0;
	myData->renderTimesHead = 0;
	for (i=0; i<FPS_AVG_SAMPLES; i++) myData->renderTimes[i] = 1/60.0;
	myData->averageFPS = 60.0;
	
	InitColorTables(myData);
	
	myData->showDebugMenu = FALSE;
    myData->displayFPS = FALSE;
    myData->displayNumParticles = FALSE;
	myData->alwaysShowTrackInfo = FALSE;
	
	myData->modeData = FMFountainModeCreateData();
    
	myData->minuteBuffer = FMAudioBufferCreate(60);
	myData->shortBuffer = FMAudioBufferCreate(5);
	bzero(myData->relativePercent, sizeof(FMAudioSpectrumData));
    	
	myData->configWindowRunning = FALSE;
	bzero(&(myData->oldSettings), sizeof(FMConfigureSettings));
	myData->configWindow = NULL;
	myData->gravitySlider = NULL;
	myData->pSharpnessSlider = NULL;
	myData->pSizeSlider = NULL;
	myData->constantTICheckBox = NULL;
	myData->fpsCheckBox = NULL;
	myData->pCountCheckBox = NULL;
	myData->colorPopup = NULL;
	
	myData->births = 0;
	myData->deaths = 0;
	
    myData->renderTimer = NULL;
    
	InitDisplayItems(myData);
	
	#ifdef DEBUG_HISTORY
	bzero(particleHistory, HISTORY_LENGTH*sizeof(short));
	historyHeadIdx = 0;
	#endif
	
	#ifdef MOTION_BLUR
	myData->motionBuffer = NULL;
	#endif
}

void InitializeTextures(VisualPluginData *myData)
{
	// assert particle sharpness to make proper texture
	SetParticleSharpness(myData, GetParticleSharpness(myData));
	
	// mark all the display items as dirty, textures will be reset when drawn
	FMListEnumerator *itemEnum;
	FMListNode *currNode;
	FMDisplayItemRef currItem;
	
	itemEnum = FMNewListEnumerator(myData->displayItems);
	
	// enumerate display items
	while (currNode = FMListEnumeratorNext(itemEnum))
	{
		currItem = (FMDisplayItemRef)(currNode->data);
		
		FMDisplayItemSetTextureDirty(currItem, TRUE);
	}
	
	FMDeleteListEnumerator(itemEnum);
}

inline void FMSwapBuffers(VisualPluginData *myData)
{                
    if (myData->glContext != NULL) aglSwapBuffers(myData->glContext);
}
	
#pragma mark -
#pragma mark Utility Functions


float RandomFloatBetween(float a, float b)
{
    return a + ((float)rand() / INT_MAX) * (b-a);
}

float TimeSinceStart(VisualPluginData *myData)
{
    return (CurrDoubleTime()-myData->startupTime);
}

float ClampAngle(float angle)
{
    while (angle < 0.0) angle += 360.0;
    while (angle > 360.0) angle -= 360.0;
    
    return angle;
}

double CurrDoubleTime()
{
    return CFAbsoluteTimeGetCurrent();
}

#define SQRT_2_PI 3.5449077

float FMGaussianProb(float mean, float stdDev, float value)
{
	return exp(-pow(value-mean, 2.0)/(2.0*stdDev*stdDev))/(stdDev*SQRT_2_PI);
}

int DetermineSizeOfFileAtURL(CFURLRef url)
{
	FSRef fileRef;
	FSCatalogInfo catInfo;
	CFStringRef urlPath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	int dataLength = CFStringGetLength(urlPath)+1;
	UInt8 *stringData = calloc(sizeof(UInt8), dataLength);
	CFIndex usedLength;
	
	CFStringGetBytes(urlPath,
					 CFRangeMake(0, CFStringGetLength(urlPath)),
					 kCFStringEncodingUTF8,
					 '?',
					 TRUE,
					 stringData,
					 dataLength,
					 &usedLength);
					 
	FSPathMakeRef(stringData, &fileRef, NULL);
	
	FSGetCatalogInfo(&fileRef, kFSCatInfoDataSizes, &catInfo, NULL, NULL, NULL);
	
	free(stringData);
	CFRelease(urlPath);
	
	return catInfo.dataLogicalSize;
}

#pragma mark -
#pragma mark Color Table API

#define DEFAULT_SPEED 3.0

void InitFallbackColorTables(VisualPluginData *myData)
{
    CFDictionaryRef defaultDict;
    const CFStringRef nameKey = CFSTR("Name");
    const CFStringRef nameValue = CFSTR("Default (Fallback)");
    
    defaultDict = CFDictionaryCreate(NULL, (const void **)&nameKey, (const void **)&nameValue, 1, NULL, NULL);

    myData->tableTable = CFArrayCreate(NULL, (const void **)&defaultDict, 1, NULL);
}

void InitColorTables(VisualPluginData *myData)
{
    CFBundleRef fmBundle = NULL;
    CFURLRef resPath = NULL;
    CFURLRef tableTableURL = NULL;
    CFReadStreamRef fileStream = NULL;
    UInt8 *buf = NULL;
    CFDataRef xmlData = NULL;
    CFPropertyListRef tList = NULL;
    CFStringRef plistErr = NULL;
    Boolean success = FALSE;
    
    int fileSize;
    CFIndex numRead;
    CFIndex bufferSize;
    
    myData->tableTable = NULL;

	// get FM bundle object
    fmBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.binaryminded.FountainMusic"));
	if (fmBundle != NULL)
    {
        // CFBundleGetBundleWithIdentifier follows the Get Rule, fmBundle has refcount of 0
        CFRetain(fmBundle);
        // now fmBundle has refcount of 1
        
        // find where FM bundle and its resources are
        resPath = CFBundleCopyResourcesDirectoryURL(fmBundle);
        if (resPath != NULL)
        {
            // CFBundleCopyResourcesDirectoryURL follows the Create Rule, resPath has refcount of 1
        
            // construct a path to where the color tables are kept
            myData->tablesPath = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, resPath, CFSTR("ColorSchemes/"), TRUE);
            if (myData->tablesPath != NULL)
            {
                // CFURLCreateCopyAppendingPathComponent follows the Create Rule, so myData->tablesPath has refcount of 1
                
                // where is my color table index file?
                tableTableURL = CFBundleCopyResourceURL(fmBundle, CFSTR("ColorIndex"), CFSTR("plist"), NULL);
                if (tableTableURL != NULL) 
                {
                    // CFBundleCopyResourceURL follows the Create Rule, so tableTableURL has refcount of 1
                
                    // i would like to read from the color table index, clean up the URL object
                    fileStream = CFReadStreamCreateWithFile(NULL, tableTableURL);
                    
                    if (fileStream != NULL)
                    {
                        // CFReadStreamCreateWithFile follows the Create Rule, so fileStream has refcount of 1
                        
                        // attempt to open the color table index
                        if (CFReadStreamOpen(fileStream))
                        {
                            fileSize = DetermineSizeOfFileAtURL(tableTableURL);

                            // alloc a temporary buffer to read the file into
                            bufferSize = fileSize;
                            buf = malloc(bufferSize);
                            
                            // read file into said buffer, clean up
                            numRead = CFReadStreamRead(fileStream, buf, bufferSize);
                            CFReadStreamClose(fileStream);
                            
                            if (numRead != -1)
                            {
                                // use said buffer buf to create a data object
                                xmlData = CFDataCreate(kCFAllocatorDefault, buf, numRead);
                                // xmlData has relative refcount of 1
                                
                                // create a plist from the xml color table data
                                tList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, xmlData, kCFPropertyListImmutable, &plistErr);
                                // tList has relative refcount of 1
                                // plistErr (if non-NULL) has relative refcount of 1
                                
                                if (plistErr == NULL)
                                {
                                    // use that as our "[color] table table"
                                    CFRetain(tList);
                                    myData->tableTable = (CFArrayRef)tList;
                                    
                                    // set some defaults
                                    myData->rgbColorTable = NULL;
                                    myData->currTableSpeed = 3.0f;
                                    myData->currTableColumn = 0.0f;
                                    SetColorTableIndex(myData, myData->currTable); // set up the initial color table
                                    
                                    success = TRUE;
                                }
                                else
                                {
                                    printf("Fountain Music Error: could not parse the color table index file:");
                                    CFShow(plistErr);
                                    printf("\n");
                                }
                            }
                            else
                            {
                                printf("Fountain Music Error: error reading data\n");
                            }
                        }
                        else
                        {
                            printf("Fountain Music Error: could not open color table index!\n");
                        }
                    }
                    else
                    {
                        printf("Fountain Music Error: could not create the file stream object\n");
                    }
                }
                else
                {
                    printf("Fountain Music Error: could not get ColorIndex plist url!\n");
                }
            }
            else
            {
                printf("Fountain Music Error: could not create color schemes dir path\n");
            }
        }
        else
        {
            printf("Fountain Music Error: could not get resource dir URL!\n");
        }
    }
    else
    {
        printf("Fountain Music Error: cannot find myself!\n");
    }
    
    if (fmBundle != NULL)
    {
        CFRelease(fmBundle);
    }
    
    if (resPath != NULL)
    {
        CFRelease(resPath);
    }
    
    if (tableTableURL != NULL)
    {
        CFRelease(tableTableURL);
    }
    
    if (fileStream != NULL)
    {
        CFRelease(fileStream);
    }
    
    if (buf != NULL)
    {
        free(buf);
    }
    
    if (xmlData != NULL)
    {
        CFRelease(xmlData);
    }
    
    if (tList != NULL)
    {
        CFRelease(tList);
    }
    
    if (plistErr != NULL)
    {
        CFRelease(plistErr);
    }
    
    if (!success) InitFallbackColorTables(myData);
}

CFStringRef CopyColorTableName(VisualPluginData *myData, int idx)
{
	CFDictionaryRef dict = CFArrayGetValueAtIndex(myData->tableTable, idx);
	CFStringRef name = CFDictionaryGetValue(dict, CFSTR("Name"));
	
	if (name == NULL)
	{
		name = CFDictionaryGetValue(dict, CFSTR("Filename"));
		
		if (name == NULL) name = CFSTR("<undefined>");
	}
	
	return CFRetain(name);
}

int FindColorTableByName(VisualPluginData *myData, CFStringRef inName)
{
	int i, count;
	CFStringRef tempName;
	
	count = CFArrayGetCount(myData->tableTable);
	
	for (i=0; i<count; i++)
	{
		tempName = CopyColorTableName(myData, i);
		CFRelease(tempName); // still in ownership of tableTable
		
		if (CFStringCompare(inName, tempName, 0) == kCFCompareEqualTo) return i;
	}
	
	return -1;
}


float GetColorTableSpeed(VisualPluginData *myData, int idx)
{
	CFDictionaryRef dict = CFArrayGetValueAtIndex(myData->tableTable, myData->currTable);
	CFNumberRef speedNum = CFDictionaryGetValue(dict, CFSTR("Speed"));
	float outSpeed;
	
	if (speedNum)
	{
		CFNumberGetValue(speedNum, kCFNumberFloat32Type, &outSpeed);
	}
	else
	{
		outSpeed = DEFAULT_SPEED;
	}
	
	return outSpeed;
}


CFURLRef CopyColorTablePath(VisualPluginData *myData, int idx)
{
	CFDictionaryRef dict;
	CFStringRef fileName;    

    if (myData->tableTable == NULL) return NULL;

	dict = CFArrayGetValueAtIndex(myData->tableTable, myData->currTable);
	fileName = CFDictionaryGetValue(dict, CFSTR("Filename"));
    
    if (fileName == NULL) return NULL;
	
	return CFURLCreateCopyAppendingPathComponent(NULL, myData->tablesPath, fileName, FALSE);
}

void MakeDefaultColorTable(VisualPluginData *myData)
{
    int i, j;
    float rgbVal[3];
    unsigned char *currPixel;

    // set the width and height of the color table
    myData->tableHeight = 256;
    myData->tableWidth = 256;

    // reallocate space for the color table
    if (myData->rgbColorTable != NULL) free(myData->rgbColorTable);
    myData->rgbColorTable = calloc(myData->tableWidth*myData->tableHeight, 4);
    
    for (i=0; i<256; i++)
    {
        for (j=0; j<256; j++)
        {
            currPixel = myData->rgbColorTable + 256*4*i + 4*j;
        
            FMConvertHSBToRGB(j/255.0, 1.0 - i/255.0, 1.0, rgbVal);
            
            currPixel[0] = roundtol(rgbVal[0]*255.0);
            currPixel[1] = roundtol(rgbVal[1]*255.0);
            currPixel[2] = roundtol(rgbVal[2]*255.0);
            currPixel[3] = 255;
        }
    }
}

void MakeColorTableFromPNG(VisualPluginData *myData, CFURLRef fileURL)
{	
    CGDataProviderRef fileProvider = NULL;
    CGImageRef tableImage = NULL;
    CGColorSpaceRef rgbSpace = NULL;
    CGContextRef bitmapContext = NULL;

	CFRetain(fileURL); // fileURL now has relative refcount of 1

    fileProvider = CGDataProviderCreateWithURL(fileURL); // fileProvider has relative refcount of 1
	if (fileProvider != NULL) 
    {
        tableImage = CGImageCreateWithPNGDataProvider(fileProvider, NULL, FALSE, kCGRenderingIntentDefault);
        // tableImage has relative refcount of 1
        
        if (tableImage != NULL) 
        {
            // set the width and height from the image
            myData->tableHeight = CGImageGetHeight(tableImage);
            myData->tableWidth = CGImageGetWidth(tableImage);

            // reallocate space for the color table
            if (myData->rgbColorTable != NULL) free(myData->rgbColorTable);
            myData->rgbColorTable = calloc(myData->tableWidth*myData->tableHeight, 4);

            // create an RGB color space object
            rgbSpace = CGColorSpaceCreateDeviceRGB(); // rgbSpace has relative refcount of 1
            if (rgbSpace != NULL) 
            {
                // make a bitmap image context to draw the table into
                bitmapContext = CGBitmapContextCreate(myData->rgbColorTable, 
                                                      myData->tableWidth,
                                                      myData->tableHeight, 
                                                      8, 
                                                      4*myData->tableWidth, 
                                                      rgbSpace, kCGImageAlphaNoneSkipLast);
                                                      
                // bitmapContext has relative refcount of 1
                if (bitmapContext != NULL)
                {
                    CGContextDrawImage(bitmapContext, CGRectMake(0.0, 0.0, myData->tableWidth, myData->tableHeight), tableImage);
                }
                else
                {
                    printf("Fountain Music Error: cannot make bitmap context\n");
                }
            }
            else
            {
                printf("Fountain Music Error: cannot create color space\n");
            }
        }
        else
        {
            printf("Fountain Music Error: cannot create png image\n");
        }
    }
    else
    {
        printf("Fountain Music Error: cannot create provider\n");
	}

	if (bitmapContext != NULL) CGContextRelease(bitmapContext);
	if (rgbSpace != NULL) CGColorSpaceRelease(rgbSpace);
	if (tableImage != NULL) CGImageRelease(tableImage);
	if (fileProvider != NULL) CGDataProviderRelease(fileProvider);
	
    // we are done with the fileURL parameter, release it
	CFRelease(fileURL);
}

void SetColorTableIndex(VisualPluginData *myData, int newIndex)
{
    // set the index in myData
	myData->currTable = newIndex;
	myData->currTableColumn = 0.0f; // reset color rotation
	
    // bounds handling
	if (myData->currTable < 0) myData->currTable = 0;
	if (myData->currTable >= CFArrayGetCount(myData->tableTable)) myData->currTable = CFArrayGetCount(myData->tableTable)-1;
	
    // given its index, get the URL for the color table image
	CFURLRef url = CopyColorTablePath(myData, myData->currTable);
	
    if (url == NULL)
    {
        MakeDefaultColorTable(myData);
        
        myData->currTableSpeed = DEFAULT_SPEED;
    }
    else
    {
        MakeColorTableFromPNG(myData, url);
        CFRelease(url);
        
        // get the color table's speed from the XML file
        myData->currTableSpeed = GetColorTableSpeed(myData, myData->currTable);
    }

    // start off in random part of color table
    myData->currTableColumn = RandomFloatBetween(0.0, myData->tableWidth-1.0);
}


float ColorTableComponentValue(VisualPluginData *myData, float frequencyPercent, int component)
{
	if (frequencyPercent > 1.0) frequencyPercent = 1.0;
	if (frequencyPercent < 0.0) frequencyPercent = 0.0;
	if (component > 4 || component < 0) return 1.0;
	
	return myData->rgbColorTable[FMImageIndex( (int)myData->currTableColumn, // cycling colors
										  (int)((myData->tableHeight-1.0) * frequencyPercent), // frequency as percent of table height
										  component, 
										  myData->tableWidth, myData->tableHeight, 4)] / 255.0;
}

#pragma mark -
#pragma mark Accessor Functions

float GetParticleSharpness(VisualPluginData *myData)
{
	return myData->sharpness;
}

void SetParticleSharpness(VisualPluginData *myData, float newSharpness)
{
    if (newSharpness > 1.0) newSharpness = 1.0;
    else if (newSharpness < 0.0) newSharpness = 0.0;
    
	if (myData->sharpness != newSharpness || myData->texName == 0)
	{
		myData->sharpness = newSharpness;
		
		if (myData->glContext)
		{
			// delete old partice texture
			if (myData->texName) glDeleteTextures(1, &myData->texName);
			
			// recreate the texture
			myData->texName = MakeRadialGradientTextureWithSharpness(newSharpness);
		}
	}
}

float GetGravityMultiplier(VisualPluginData *myData)
{
	return myData->currGravity/GRAV;
}

void SetGravityMultiplier(VisualPluginData *myData, float mult)
{
	if (mult < 0.25) mult = 0.25;
	if (mult > 4.0) mult = 4.0;

	myData->currGravity = GRAV*mult;
	
	if (myData->supply != NULL)
	{
		FMListEnumerator *enumer = FMNewListEnumerator(myData->supply->usedList);
		
		while (FMListEnumeratorNext(enumer))
		{
			((Particle3D *)FMListEnumeratorCurrent(enumer)->data)->accel.y = myData->currGravity;
		}
		
		FMDeleteListEnumerator(enumer);
	}
}

float GetParticleSize(VisualPluginData *myData)
{
	return myData->particleSize;
}

void SetParticleSize(VisualPluginData *myData, float newSize)
{
    if (newSize < 0.1) newSize = 0.1;
	if (newSize > 3.0) newSize = 3.0;

	myData->particleSize = newSize;
}

void SetShowsFPSCounter(VisualPluginData *myData, Boolean doesShow)
{
	myData->displayFPS = doesShow;
	ShowFPSMeter(myData, doesShow);
}

void SetShowsParticleCounter(VisualPluginData *myData, Boolean doesShow)
{
	myData->displayNumParticles = doesShow;
	ShowParticleMeter(myData, doesShow);
}

Boolean GetConstantTrackInfo(VisualPluginData *myData)
{
	return myData->alwaysShowTrackInfo;
}

void SetConstantTrackInfo(VisualPluginData *myData, Boolean isConstant)
{
	myData->alwaysShowTrackInfo = isConstant;
	ShowTrackInfo(myData, myData->alwaysShowTrackInfo);
}

Float32 GetTextSize(VisualPluginData *myData)
{
	return myData->textSize;
}

void SetTextSize(VisualPluginData *myData, Float32 inSize)
{
    if (inSize < 2.0) inSize = 2.0;

	myData->textSize = inSize;
	
	FMDisplayItemSetTextSize(myData->namedItems.artistDispItem, myData->textSize);
	FMDisplayItemSetTextSize(myData->namedItems.albumDispItem, myData->textSize);
	FMDisplayItemSetTextSize(myData->namedItems.titleDispItem, myData->textSize);
	
	LayoutDisplayItems(myData);
	
	ShowTrackInfo(myData, myData->alwaysShowTrackInfo);
}

CFStringRef GetCurrentVersion(VisualPluginData *myData)
{
	CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.binaryminded.FountainMusic"));
	return CFBundleGetValueForInfoDictionaryKey(pluginBundle, CFSTR("CFBundleShortVersionString"));
}

