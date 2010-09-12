/* 
 *  hsbrgb.h
 *  FountainMusic
*/

#include <ApplicationServices/ApplicationServices.h>

#ifndef HSBRGB_H
#define HSBRGB_H

void FMConvertHSBToRGB(CGFloat h, CGFloat s, CGFloat b, CGFloat rgb[3]);

#endif