/*
 *  FMDisplayItem.c
 *  FountainMusic
 *
 *  Copyright 2005-2011 Brian Moore
 *
 */

#include "FMDisplayItem.h"
#import <OpenGL/gl.h>

#ifndef MAX
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a, b) ((a)<(b)?(a):(b))
#endif

#define kFMMaxItemHeight 768
#define kFMMaxItemWidth 1024
#define kFMDisplayItemStringMaxLength 256

struct FMDisplayItem
{
	int id;

	float x;
	float y;
	
	FMDisplayItemAlignmentFlags alignment;
	
	CFStringRef text;
	
	float timeToDie;
	float fadeTime;
	
	int bitmapWidth;
	int bitmapHeight;
	UInt8 *bitmapData;
	
	CGFloat textSize;
	CFMutableDictionaryRef textStyle;
	CTLineRef textLine;
	
	CGFloat textOffsetX, textOffsetY;

	// graphics
	Boolean graphicsSetUp;
	GLuint texName;
	Boolean textureDirty;
	CGContextRef bitmapCtx;
	CGColorSpaceRef colorSpace;
	
};

float FadeOutValueForTime(float timeRemaining, float fadeDuration);
int u_strncmp(UniChar *s1, UniChar *s2, int strLen);

// private API
int FMDisplayItemGetBitmapWidth(FMDisplayItemRef item);
int FMDisplayItemGetBitmapHeight(FMDisplayItemRef item);
void FMDisplayItemSetBitmapSize(FMDisplayItemRef item, int width, int height);
void FMDisplayItemRenderText(FMDisplayItemRef item);
void FMSetStyleTextSize(CFMutableDictionaryRef style, CGFloat size);

// item creation API
FMDisplayItemRef FMDisplayItemCreate()
{
	static int nextID = 0;
	FMDisplayItemRef newItem = calloc(1, sizeof(struct FMDisplayItem));
	
	newItem->id = nextID++;
	
	newItem->x = 0.0;
	newItem->y = 0.0;
	 
	newItem->alignment = kFMDisplayItemLeft | kFMDisplayItemBottom;
	
	newItem->text = NULL;
	
	newItem->timeToDie = 0.0;
	newItem->fadeTime = 0.0;
	
	newItem->bitmapData = NULL;
	newItem->texName = 0;
	
	FMDisplayItemSetBitmapSize(newItem, 100, 20);
	
	newItem->graphicsSetUp = FALSE;
	newItem->colorSpace = NULL;
	newItem->textureDirty = TRUE;
	
	// setup style
	newItem->textSize = 12.0; // default
	CFMutableDictionaryRef style;
	style = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	// white text
	CFDictionarySetValue(style, kCTForegroundColorAttributeName, CGColorGetConstantColor(kCGColorWhite));
	
	// outlined with black
	CGFloat strokeWidthF = -1.5;
	CFNumberRef strokeWidth = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &strokeWidthF);
	CFDictionarySetValue(style, kCTStrokeWidthAttributeName, strokeWidth);
	CFRelease(strokeWidth);
	
	CFDictionarySetValue(style, kCTStrokeColorAttributeName, CGColorGetConstantColor(kCGColorBlack));
	
	// system UI size N font
	FMSetStyleTextSize(style, newItem->textSize);
	
	newItem->textStyle = style;
	newItem->textLine = NULL;
	FMDisplayItemSetStringValueCFString(newItem, CFSTR("")); // will setup the text line
	
	return newItem;
}

void FMDisplayItemDelete(FMDisplayItemRef item)
{
	CFRelease(item->textStyle);
	if (item->graphicsSetUp) FMDisplayItemCleanupGraphics(item);
	free(item->bitmapData);
	free(item);
}

void FMDisplayItemSetPosition(FMDisplayItemRef item, float newX, float newY)
{
	item->x = newX;
	item->y = newY;
}

float FMDisplayItemGetFadeTime(FMDisplayItemRef item)
{
	return item->fadeTime;
}
void FMDisplayItemSetFadeTime(FMDisplayItemRef item, float newFadeTime)
{
	item->fadeTime = newFadeTime;
}

float FMDisplayItemGetTimeToDie(FMDisplayItemRef item)
{
	return item->timeToDie;
}
void FMDisplayItemSetTimeToDie(FMDisplayItemRef item, float newTime)
{
	item->timeToDie = newTime;
}

CGFloat FMDisplayItemGetTextSize(FMDisplayItemRef item)
{
	return item->textSize;
}

void FMDisplayItemSetTextSize(FMDisplayItemRef item, CGFloat inSize)
{
	CGFloat oldSize = FMDisplayItemGetTextSize(item);
	
	item->textSize = inSize;
	
	if (oldSize != FMDisplayItemGetTextSize(item))
	{
		FMSetStyleTextSize(item->textStyle, item->textSize);
		
		item->textureDirty = TRUE;
	}
}

void FMSetStyleTextSize(CFMutableDictionaryRef style, CGFloat size) {
	CTFontRef font = CTFontCreateUIFontForLanguage(kCTFontEmphasizedSystemFontType, size, NULL);
	
	CFDictionarySetValue(style, kCTFontAttributeName, font);
	CFRelease(font);	
}

void FMDisplayItemStep(FMDisplayItemRef item, float deltaT)
{
	if (item->timeToDie > 0.0)
	{
		item->timeToDie -= deltaT;
		if (item->timeToDie < 0.0) item->timeToDie = 0.0;
	}
	
}

float FMDisplayItemAlpha(FMDisplayItemRef item)
{
	return FadeOutValueForTime(item->timeToDie, item->fadeTime);
}

int FMDisplayItemGetBitmapWidth(FMDisplayItemRef item)
{
	return item->bitmapWidth;
}

int FMDisplayItemGetBitmapHeight(FMDisplayItemRef item)
{
	return item->bitmapHeight;
}

void FMDisplayItemSetBitmapSize(FMDisplayItemRef item, int width, int height)
{
	height = height > kFMMaxItemHeight ? kFMMaxItemHeight : height;
	width = width > kFMMaxItemWidth ? kFMMaxItemWidth : width;

	if (width > item->bitmapWidth || height > item->bitmapHeight)
	{
		item->bitmapHeight = height;
		item->bitmapWidth = width;
		
		if (item->bitmapData) free(item->bitmapData);
		
		item->bitmapData = calloc(item->bitmapHeight * item->bitmapWidth, 4*sizeof(UInt8));
		
		if (item->graphicsSetUp)
		{
			CGContextRelease(item->bitmapCtx);
		
			item->bitmapCtx = CGBitmapContextCreate(item->bitmapData,
													item->bitmapWidth,
													item->bitmapHeight,
													8, // bits per component
													item->bitmapWidth*4,
													item->colorSpace,
													kCGImageAlphaPremultipliedLast);
													
			item->textureDirty = TRUE;
		}
	}
}

void FMDisplayItemSetStringValueMacRoman(FMDisplayItemRef item, const char *newText)
{
	int stringLength;
	
	stringLength = MIN(kFMDisplayItemStringMaxLength, strlen(newText));
	
	// convert from MacRoman to UTF-16
	CFStringRef uniString = CFStringCreateWithBytes(NULL, (UInt8 *)newText, stringLength, kCFStringEncodingMacRoman, TRUE);
	
	FMDisplayItemSetStringValueCFString(item, uniString);
		
	CFRelease(uniString);
}

void FMDisplayItemSetStringValueCFString(FMDisplayItemRef item, CFStringRef str)
{	
	// set the text field
	if (item->text) {
		CFRelease(item->text);
		item->text = NULL;
	}
	
	if (str) {
		item->text = CFStringCreateCopy(kCFAllocatorDefault, str);
	}
	
	// set the line field
	if (item->textLine) {
		CFRelease(item->textLine);
		item->textLine = NULL;
	}
	
	if (item->text) {
		CFAttributedStringRef attributedString = CFAttributedStringCreate(kCFAllocatorDefault, item->text, item->textStyle);
		item->textLine = CTLineCreateWithAttributedString(attributedString);
		CFRelease(attributedString);
	}
	
	// mark the texture as needing to be updated
	item->textureDirty = TRUE;
}

void FMDisplayItemSetTextAlignment(FMDisplayItemRef item, FMDisplayItemAlignmentFlags flags)
{
	item->alignment = flags;
	
	item->textureDirty = TRUE;
}

void FMDisplayItemSetTextureDirty(FMDisplayItemRef item, Boolean flag)
{
	item->textureDirty = flag;
}

void FMDisplayItemSetupGraphics(FMDisplayItemRef item)
{
	if (!item->graphicsSetUp)
	{
		item->graphicsSetUp = TRUE;
		
		glGenTextures(1, &(item->texName));
		
		item->colorSpace = CGColorSpaceCreateDeviceRGB();
		if (item->colorSpace == NULL) fprintf(stderr, "Fountain Music Error: Could not create color space!\n");
		
		item->bitmapCtx = CGBitmapContextCreate(item->bitmapData,
											    item->bitmapWidth,
											    item->bitmapHeight,
											    8, // bits per component
											    item->bitmapWidth*4,
											    item->colorSpace,
											    kCGImageAlphaPremultipliedLast);
		if (item->bitmapCtx == NULL) fprintf(stderr, "Fountain Music Error: Could not create bitmap context!\n");
		
		item->textureDirty = TRUE;
	}
}

void FMDisplayItemCleanupGraphics(FMDisplayItemRef item)
{
	if (item->graphicsSetUp)
	{
		item->graphicsSetUp = FALSE;
		
		glDeleteTextures(1, &(item->texName));
		
		CGContextRelease(item->bitmapCtx);
		CGColorSpaceRelease(item->colorSpace);
	}
}

#define TXT_AA_PADDING (2)

void FMDisplayItemRenderText(FMDisplayItemRef item)
{
	if (item->textureDirty && item->graphicsSetUp)
	{
		CGRect textRect;
		CGFloat typoWidth, ascent, descent;
		
		CGContextSetTextPosition(item->bitmapCtx, 0.0, 0.0);
		
		// obtain text metrics
		textRect = CTLineGetImageBounds(item->textLine, item->bitmapCtx);
		typoWidth = CTLineGetTypographicBounds(item->textLine, &ascent, &descent, NULL);
		
		CGFloat offsetX, offsetY;
		int bitmapW, bitmapH;
		
		offsetX = lround(TXT_AA_PADDING + -textRect.origin.x);
		offsetY = lround(TXT_AA_PADDING + -textRect.origin.y);
		
		bitmapW = lround(textRect.size.width + 2*TXT_AA_PADDING);
		bitmapH = lround(textRect.size.height + 2*TXT_AA_PADDING);
		
		item->textOffsetX = -offsetX;
		item->textOffsetY = -offsetY;
		
		//printf("tf: %f, %f\n", item->textOffsetX, item->textOffsetY);
		
		if (bitmapW > 0 && bitmapH > 0)
		{
			//
			// first render into bitmap
			//
			// resize bitmap and backing as needed
			FMDisplayItemSetBitmapSize(item, bitmapW, bitmapH);
			
			// zero the bitmap
			memset(item->bitmapData, 0, item->bitmapHeight * item->bitmapWidth * 4);
			
			CGContextSetShadow(item->bitmapCtx, CGSizeMake(2.0, -2.0), 1.0);
			
			CGContextSetTextPosition(item->bitmapCtx, -item->textOffsetX, -item->textOffsetY);
			CTLineDraw(item->textLine, item->bitmapCtx);
			CGContextFlush(item->bitmapCtx);
			
			//
			// then load the bitmap into the GL texture
			//
			glEnable(GL_TEXTURE_RECTANGLE_EXT);
			glBindTexture(GL_TEXTURE_RECTANGLE_EXT, item->texName);
			
			glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0,
						 GL_RGBA,
						 item->bitmapWidth,
						 item->bitmapHeight,
						 0,
						 GL_RGBA,
						 GL_UNSIGNED_BYTE,
						 item->bitmapData);
						 
			glDisable(GL_TEXTURE_RECTANGLE_EXT);
			
			item->textureDirty = FALSE;
		}
	}
}

void FMDisplayItemDraw(FMDisplayItemRef item)
{
	if (!item->graphicsSetUp) FMDisplayItemSetupGraphics(item);

	if (item->timeToDie > 0.0)
	{
		FMDisplayItemRenderText(item);
		
		float px, py;
		
		px = item->x + item->textOffsetX;
		py = item->y + item->textOffsetY;
		
		glEnable(GL_TEXTURE_RECTANGLE_EXT);
		glBindTexture(GL_TEXTURE_RECTANGLE_EXT, item->texName);
		
		// setup good blending for the text texture
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glColor4f(1.0, 1.0, 1.0, FMDisplayItemAlpha(item));
					  
		glBegin(GL_QUADS);
		
			glTexCoord2f(0.0, 0.0);
			glVertex2f(px, py+item->bitmapHeight);
			
			glTexCoord2f(item->bitmapWidth, 0.0);
			glVertex2f(px+item->bitmapWidth, py+item->bitmapHeight);
			
			glTexCoord2f(item->bitmapWidth, item->bitmapHeight);
			glVertex2f(px+item->bitmapWidth, py);
			
			glTexCoord2f(0.0, item->bitmapHeight);
			glVertex2f(px, py);
		
		glEnd();
		
		glDisable(GL_TEXTURE_RECTANGLE_EXT);
	}
}

// other
float FadeOutValueForTime(float timeRemaining, float fadeDuration)
{
    float fraction;
    
    if (timeRemaining < fadeDuration) 
    {
		fraction = timeRemaining/fadeDuration;
    }
    else fraction = 1.0;
	
    return fraction;
}

int u_strncmp(UniChar *s1, UniChar *s2, int strLen)
{
	int i;
	int cmp;
	
	for (i=0; i<strLen; i++)
	{
		cmp = s2[i] - s1[i];
		if (cmp != 0) return cmp;
	}
	
	return 0;
}
