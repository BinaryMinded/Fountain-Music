/* 
 *  hsbrgb.c
 *  FountainMusic
*/

#import <Carbon/Carbon.h>


void FMConvertHSBToRGB(CGFloat h, CGFloat s, CGFloat b, CGFloat rgb[3])
{
	// yeah, so this is the only function Mac OS X provides in pure C for HSL/RGB conversion, argh.
	HSVColor hsvColor;
	
	hsvColor.hue = floor(h*(CGFloat)kMaximumSmallFract);
	hsvColor.saturation = floor(s*(CGFloat)kMaximumSmallFract);
	hsvColor.value = floor(b*(CGFloat)kMaximumSmallFract);
	
	RGBColor rgbColor;
	HSV2RGB(&hsvColor, &rgbColor);
	
	rgb[0] = (CGFloat)rgbColor.red/(CGFloat)kMaximumSmallFract;
	rgb[1] = (CGFloat)rgbColor.green/(CGFloat)kMaximumSmallFract;
	rgb[2] = (CGFloat)rgbColor.blue/(CGFloat)kMaximumSmallFract;
}
