/*
 *  FMConfigure.c
 *  FountainMusic
 *
 *  Copyright 2004, 2005, 2006 Brian Moore
 *
 */

#include "FMConfigure.h"

#define kFMOKButtonID 3
#define kFMCancelButtonID 4
#define kFMDefaultButtonID 5

#define kFMSliderSig 'slid'
#define kFMGravitySliderID 6
#define kFMParticleSharpnessSliderID 7
#define kFMParticleSizeSliderID 8

#define kFMPopupButtonSig 'popu'

#define kFMCheckBoxSig 'chec'
#define kFMFPSCheckBoxID 10
#define kFMParticleCountCheckBoxID 11
#define kFMConstantTrackInfoCheckBoxID 12

#define kFMColorTablePopupID 13
#define kFMTextSizePopupID 14

#define kFMNullCommandID 42

#define kNumTextSizeIncrements 10
int textSizeIncrements[] = { 9, 10, 11, 12, 13, 14, 18, 24, 36, 48 };

void FMResetToDefaults(VisualPluginData *myData);
void FMResetToOldSettings(VisualPluginData *myData);
void RunBGColorPicker(VisualPluginData *myData);
void FMConfigSetupUI(VisualPluginData *myData);
void FMSaveCurrentSettings(VisualPluginData *myData);

OSStatus WindowEventHandler(EventHandlerCallRef inHandlerCallRef, EventRef event, void *userData);

#pragma mark -

OSStatus FMInitializeConfig(VisualPluginData *myData)
{
	OSErr err = noErr;

	if (myData->configWindow == NULL)
	{
		// get my bundle
		CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.binaryminded.FountainMusic"));
		if (!pluginBundle)
		{
			printf("Fountain Music Error %i: Could not get plugin bundle in FMRunConfig!\n", err);
			return -1;
		}
		
		
		IBNibRef configNibRef;
		// create nib reference
		err = CreateNibReferenceWithCFBundle(pluginBundle, CFSTR("Config"), &configNibRef);
		if (err != noErr) 
		{
			printf("Fountain Music Error %i: Could not create nib ref in FMRunConfig!\n", err);
			return err;
		}
		
		// pull the window out of the nib
		err = CreateWindowFromNib(configNibRef, CFSTR("Window"), &(myData->configWindow));
		if (err != noErr) 
		{
			printf("Fountain Music Error %i: Could not create window in FMRunConfig!\n", err);
			return err;
		}
		
		// throw away the nib
		DisposeNibReference(configNibRef);
		
		EventTypeSpec controlEvent={kEventClassControl,kEventControlHit};
		// install event handlers
		InstallWindowEventHandler(myData->configWindow, NewEventHandlerUPP(WindowEventHandler), 1,
								  &controlEvent, myData, NULL);
								  
		ControlID gravitySliderID;
		gravitySliderID.id = kFMGravitySliderID;
		gravitySliderID.signature = kFMSliderSig;
		
		ControlID sharpSliderID;
		sharpSliderID.id = kFMParticleSharpnessSliderID;
		sharpSliderID.signature = kFMSliderSig;
		
		ControlID sizeSliderID;
		sizeSliderID.id = kFMParticleSizeSliderID;
		sizeSliderID.signature = kFMSliderSig;
		
		ControlID fpsCheckID;
		fpsCheckID.id = kFMFPSCheckBoxID;
		fpsCheckID.signature = kFMCheckBoxSig;
		
		ControlID pCountCheckID;
		pCountCheckID.id = kFMParticleCountCheckBoxID;
		pCountCheckID.signature = kFMCheckBoxSig;
				
		ControlID constantTICheckID;
		constantTICheckID.id = kFMConstantTrackInfoCheckBoxID;
		constantTICheckID.signature = kFMCheckBoxSig;
		
		ControlID colorPopupID;
		colorPopupID.id = kFMColorTablePopupID;
		colorPopupID.signature = kFMPopupButtonSig;
		
		ControlID textSizePopupID;
		textSizePopupID.id = kFMTextSizePopupID;
		textSizePopupID.signature = kFMPopupButtonSig;
								  
		GetControlByID(myData->configWindow, &gravitySliderID, &myData->gravitySlider);
		GetControlByID(myData->configWindow, &sharpSliderID, &myData->pSharpnessSlider);
		GetControlByID(myData->configWindow, &sizeSliderID, &myData->pSizeSlider);
		
		GetControlByID(myData->configWindow, &fpsCheckID, &myData->fpsCheckBox);
		GetControlByID(myData->configWindow, &pCountCheckID, &myData->pCountCheckBox);
		GetControlByID(myData->configWindow, &constantTICheckID, &myData->constantTICheckBox);
		
		GetControlByID(myData->configWindow, &colorPopupID, &myData->colorPopup);
		GetControlByID(myData->configWindow, &textSizePopupID, &myData->textSizePopup);
	}
	
	return err;
}

OSStatus FMRunConfig(VisualPluginData *myData)
{
	///// Here we should be checking if the window is already open, and if it is simply bring it to the front
	
	OSErr err = noErr;

	if (myData->configWindowRunning)
	{
		// simply bring window to front, sync
		SelectWindow(myData->configWindow);
		FMConfigSyncUI(myData);
	}
	else
	{
		myData->configWindowRunning = TRUE;
		
		// create window 
		if (myData->configWindow == NULL) FMInitializeConfig(myData);

		// setup and synchronize UI with data
		FMConfigSetupUI(myData);
		FMConfigSyncUI(myData);
		
		// show the window
		ShowWindow(myData->configWindow);
		
		// save values at open
		FMSaveCurrentSettings(myData);
	}
	return err;
}

OSStatus FMCloseConfig(VisualPluginData *myData)
{
	if (myData->configWindowRunning && myData->configWindow != NULL)
	{
		myData->configWindowRunning = FALSE;
		
		// hide away config window, because someone's gonna get you
		HideWindow(myData->configWindow);
	}
	
	return noErr;
}

void FMSaveCurrentSettings(VisualPluginData *myData)
{
	myData->oldSettings.particleSize = myData->particleSize;
	myData->oldSettings.particleSharpness = myData->sharpness;
	myData->oldSettings.gravityMultiplier = GetGravityMultiplier(myData);
	myData->oldSettings.displayFPS = myData->displayFPS;
	myData->oldSettings.displayNumParticles = myData->displayNumParticles;
	myData->oldSettings.constantTI = myData->alwaysShowTrackInfo;
	myData->oldSettings.colorIdx = myData->currTable;
	myData->oldSettings.textSize = GetTextSize(myData);
}

OSStatus WindowEventHandler(EventHandlerCallRef inHandlerCallRef, EventRef event, void *userData)
{
	VisualPluginData *myData = userData;
	ControlID hitID;
	ControlRef controlHit;
	GetEventParameter(event, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &controlHit);
	GetControlID(controlHit,&hitID);
	float floatValue;
	
	switch(hitID.id)
	{
		case kFMOKButtonID:
			FMCloseConfig(myData);
			break;
		
		case kFMCancelButtonID:
			FMResetToOldSettings(myData);
			FMCloseConfig(myData);
			break;
		
		case kFMDefaultButtonID:
			FMResetToDefaults(myData);
			break;
			
		case kFMGravitySliderID:
			floatValue = GetControlValue(myData->gravitySlider)/1000.0;
			SetGravityMultiplier(myData, pow(2.0, floatValue));
			break;
			
		case kFMParticleSizeSliderID:
			floatValue = GetControlValue(myData->pSizeSlider)/1000.0;
			SetParticleSize(myData, floatValue);
			break;
			
		case kFMParticleSharpnessSliderID:
			floatValue = GetControlValue(myData->pSharpnessSlider)/1000.0;
			SetParticleSharpness(myData, floatValue);
			break;
			
		case kFMFPSCheckBoxID:
			SetShowsFPSCounter(myData, GetControlValue(myData->fpsCheckBox));
			break;
		
		case kFMParticleCountCheckBoxID:
			SetShowsParticleCounter(myData, GetControlValue(myData->pCountCheckBox));
			break;
					
		case kFMColorTablePopupID:
			SetColorTableIndex(myData, GetControlValue(myData->colorPopup)-1);
			break;
			
		case kFMConstantTrackInfoCheckBoxID:
			SetConstantTrackInfo(myData, GetControlValue(myData->constantTICheckBox));
			break;
			
		case kFMTextSizePopupID:
			SetTextSize(myData, textSizeIncrements[GetControlValue(myData->textSizePopup)-1]);
	}
	
	return noErr;
}

// basically the below function sets up the menu buttons
void FMConfigSetupUI(VisualPluginData *myData) 
{
	if (myData->configWindowRunning)
	{		
		// sync color table popup button
		MenuRef colorMenu = GetControlPopupMenuHandle(myData->colorPopup);
		
		DeleteMenuItems(colorMenu, 1, CountMenuItems(colorMenu));
		
		CFStringRef currString;
		int i, colorCount = CFArrayGetCount(myData->tableTable);
		for (i=0; i<colorCount; i++)
		{
			currString = CopyColorTableName(myData, i);
			
			AppendMenuItemTextWithCFString(colorMenu, currString, 0, kFMNullCommandID, NULL);
			
			CFRelease(currString);
		}
		
		SetControl32BitMaximum(myData->colorPopup, colorCount);
		
		// create text size menu
		MenuRef textSizeMenu = GetControlPopupMenuHandle(myData->textSizePopup);
		CFStringRef textSizeStr;
		
		DeleteMenuItems(textSizeMenu, 1, CountMenuItems(textSizeMenu));
		
		for (i=0; i<kNumTextSizeIncrements; i++)
		{
			textSizeStr = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ipt"), textSizeIncrements[i]);
			
			AppendMenuItemTextWithCFString(textSizeMenu, textSizeStr, 0, kFMNullCommandID, NULL);
			
			CFRelease(textSizeStr);
		}
		
		SetControl32BitMaximum(myData->textSizePopup, kNumTextSizeIncrements);
		
		
	}
}

// synchronize volatile values
void FMConfigSyncUI(VisualPluginData *myData)
{
	if (myData->configWindowRunning)
	{
		float gravityExponent = log(GetGravityMultiplier(myData))/log(2.0);
		
		SetControl32BitValue(myData->gravitySlider, gravityExponent*1000);
		
		SetControl32BitValue(myData->pSizeSlider, myData->particleSize*1000);
		SetControl32BitValue(myData->pSharpnessSlider, myData->sharpness*1000);
		
		SetControl32BitValue(myData->pCountCheckBox, myData->displayNumParticles);
		SetControl32BitValue(myData->fpsCheckBox, myData->displayFPS);
		
		SetControl32BitValue(myData->constantTICheckBox, myData->alwaysShowTrackInfo);
		
		SetControl32BitValue(myData->colorPopup, myData->currTable+1);
		
		int i, index = -1;
		Float32 currSize = GetTextSize(myData);
		for (i=0; i<kNumTextSizeIncrements; i++)
		{
			if (fabs(currSize - textSizeIncrements[i]) < 0.00001)
			{
				index = i;
				break;
			}
		}
		if (index >= 0) SetControl32BitValue(myData->textSizePopup, index+1);
		else SetControl32BitValue(myData->textSizePopup, 5);
	}
}


void FMResetToDefaults(VisualPluginData *myData)
{
	SetParticleSharpness(myData, 0.0);
	SetParticleSize(myData, 0.2);
	SetGravityMultiplier(myData, 1.0);
	SetShowsFPSCounter(myData, FALSE);
	SetShowsParticleCounter(myData, FALSE);
	SetConstantTrackInfo(myData, FALSE);
	SetColorTableIndex(myData, 0);
	SetTextSize(myData, 13.0);
	
	FMConfigSyncUI(myData);
}

void FMResetToOldSettings(VisualPluginData *myData)
{
	SetParticleSharpness(myData, myData->oldSettings.particleSharpness);
	SetParticleSize(myData, myData->oldSettings.particleSize);
	SetGravityMultiplier(myData, myData->oldSettings.gravityMultiplier);
	SetShowsFPSCounter(myData, myData->oldSettings.displayFPS);
	SetShowsParticleCounter(myData, myData->oldSettings.displayNumParticles);
	SetConstantTrackInfo(myData, myData->oldSettings.constantTI);
	SetColorTableIndex(myData, myData->oldSettings.colorIdx);
	SetTextSize(myData, myData->oldSettings.textSize);
}