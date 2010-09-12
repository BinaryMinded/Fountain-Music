//
//  InstallFountainMusicAppDelegate.h
//  InstallFountainMusic
//
//  Created by Brian Moore on 9/11/10.
//  Copyright 2010 Personal. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface InstallFountainMusicAppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *window;
	BOOL productIsInstalled;
	NSUInteger selectedOptionIndex;
	NSString *installButtonTitle;
	
	NSInteger itunesesNeedingToQuit;
}

@property (assign) IBOutlet NSWindow *window;

@property (assign) BOOL productIsInstalled;
@property (assign) NSUInteger selectedOptionIndex;
@property (readonly) NSString *installButtonTitle;
@property (assign) NSInteger itunesesNeedingToQuit;
@property (readonly) BOOL isWaitingForiTunesTermination;

- (void)doInstall:(id)sender;

- (BOOL)installProductToPath:(NSString *)path;
- (BOOL)uninstallProduct;

@end
