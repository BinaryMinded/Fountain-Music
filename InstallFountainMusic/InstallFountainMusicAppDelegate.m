//
//  InstallFountainMusicAppDelegate.m
//  InstallFountainMusic
//
//  Created by Brian Moore on 9/11/10.
//  Copyright 2010 Personal. All rights reserved.
//

#import "InstallFountainMusicAppDelegate.h"

static NSString *kProductFilename = @"Fountain Music.bundle";
static NSString *kUserInstallPath = @"~/Library/iTunes/iTunes Plug-ins/";
static NSString *kSystemInstallPath = @"/Library/iTunes/iTunes Plug-ins/";

static NSString *kWaitingForiTunesTerminationContext = @"kWaitingForiTunesTerminationContext";

@implementation InstallFountainMusicAppDelegate
@synthesize productIsInstalled;
@synthesize selectedOptionIndex;
@synthesize itunesesNeedingToQuit;

@synthesize window;

+ (NSSet *)keyPathsForValuesAffectingInstallButtonTitle {
	return [NSSet setWithObjects:@"selectedOptionIndex", @"productIsInstalled", nil];
}

+ (NSSet *)keyPathsForValuesAffectingIsWaitingForiTunesTermination {
	return [NSSet setWithObject:@"itunesesNeedingToQuit"];
}

- (BOOL)isWaitingForiTunesTermination {
	return [self itunesesNeedingToQuit] > 0;
}

- (NSArray *)findExistingProductPaths {
	NSArray *possibleLocations = [NSArray arrayWithObjects:
								  kUserInstallPath,
								  kSystemInstallPath, nil];
	
	NSMutableArray *existingPaths = [NSMutableArray array];
	NSFileManager *fm = [NSFileManager defaultManager];
	
	for (NSString *possibleBase in possibleLocations) {
		NSString *possiblePath = [[possibleBase stringByExpandingTildeInPath] 
								  stringByAppendingPathComponent:kProductFilename];
		
		if ([fm fileExistsAtPath:possiblePath]) {
			[existingPaths addObject:possiblePath];
		}
	}
	
	return existingPaths;
}

- (void)refreshProductIsInstalled {
	[self setProductIsInstalled:[[self findExistingProductPaths] count]>0];
	
	if ([self selectedOptionIndex] == 2 && ![self productIsInstalled]) {
		[self setSelectedOptionIndex:1];
	}
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	[self refreshProductIsInstalled];
	
	[self setSelectedOptionIndex:0];
}


- (NSString *)productSourcePath {
	return [[NSBundle mainBundle] pathForResource:[kProductFilename stringByDeletingPathExtension] ofType:[kProductFilename pathExtension]];
}

- (NSImage *)productIcon {
	return [[NSWorkspace sharedWorkspace] iconForFile:[self productSourcePath]];
}


- (NSString *)installButtonTitle {
	return [self selectedOptionIndex] == 2 ? @"Remove" : ([self productIsInstalled] ? @"Reinstall" : @"Install");
}

- (BOOL)iTunesIsRunning {
	NSArray *ituneses = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.apple.iTunes"];
	
	return [ituneses count] > 0;
}

- (void)runHappySheetWithTitle:(NSString *)title message:(NSString *)message restartMessage:(NSString *)restartMessage {
	NSAlert *alert = [NSAlert alertWithMessageText:title 
									 defaultButton:[self iTunesIsRunning] ? @"Restart iTunes" : @"Open iTunes"
								   alternateButton:@"Quit Installer" 
									   otherButton:nil 
						 informativeTextWithFormat:@"%@ %@", message, [self iTunesIsRunning] ? restartMessage : @""];
	
	[alert setAlertStyle:NSInformationalAlertStyle];
	[alert setIcon:[self productIcon]];
	
	[alert beginSheetModalForWindow:[self window] 
					  modalDelegate:self 
					 didEndSelector:@selector(userDidDismissHappySheet:returnCode:contextInfo:) 
						contextInfo:NULL];
}

- (BOOL)openiTunesAndQuitIfPossible {
	if ([self itunesesNeedingToQuit] == 0) {
		
		// start iTunes
		[[NSWorkspace sharedWorkspace] launchAppWithBundleIdentifier:@"com.apple.iTunes" 
															 options:0
									  additionalEventParamDescriptor:NULL 
													launchIdentifier:NULL];			
		
		// and quit
		[NSApp terminate:self];
		
		return YES;
	} else {
		return NO;
	}
}

- (void)userDidDismissHappySheet:(NSAlert *)alert returnCode:(NSUInteger)code contextInfo:(void*)info {
	if (code == NSOKButton) {
		// close iTunes if open
		NSArray *ituneses = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.apple.iTunes"];
		
		[self setItunesesNeedingToQuit:[ituneses count]];
		for (NSRunningApplication *itunes in ituneses) {
			[itunes addObserver:self forKeyPath:@"isTerminated" options:0 context:kWaitingForiTunesTerminationContext];
			[itunes terminate];
			[itunes retain];
		}
		
		[self openiTunesAndQuitIfPossible];

	} else {
		[NSApp terminate:self];
	}
}


- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if (context == kWaitingForiTunesTerminationContext) {
		if ([object isTerminated]) {
			[self setItunesesNeedingToQuit:[self itunesesNeedingToQuit]-1];
			[object removeObserver:self forKeyPath:@"isTerminated"];
			[object autorelease];
		}
		
		[self openiTunesAndQuitIfPossible];
	}
}

- (void)runSadSheetWithTitle:(NSString *)title message:(NSString *)message {
	NSAlert *alert = [NSAlert alertWithMessageText:title 
									 defaultButton:@"Quit Installer" 
								   alternateButton:nil 
									   otherButton:nil 
						 informativeTextWithFormat:@"%@",message];
	
	[alert setAlertStyle:NSCriticalAlertStyle];
	
	[alert beginSheetModalForWindow:[self window] 
					  modalDelegate:self 
					 didEndSelector:@selector(userDidDismissSadSheet:returnCode:contextInfo:) 
						contextInfo:NULL];
}

- (void)userDidDismissSadSheet:(NSAlert *)alert returnCode:(NSUInteger)code contextInfo:(void*)info {
	[NSApp terminate:self];
}

- (void)doInstall:(id)sender {
	switch ([self selectedOptionIndex]) {
		case 0:
			if ([self installProductToPath:kUserInstallPath]) {
				[self runHappySheetWithTitle:@"Install Successful" 
									 message:@"Fountain Music has been installed."
							  restartMessage:@"You must restart iTunes to use it."];
			} else {
				[self runSadSheetWithTitle:@"Install Failed" message:@"There was an error installing Fountain Music."];
			}
			
			break;

		case 1:
			
			if ([self installProductToPath:kSystemInstallPath]) {
				[self runHappySheetWithTitle:@"Install Successful" 
									 message:@"Fountain Music has been installed."
							  restartMessage:@"You must restart iTunes to use it."];
			} else {
				[self runSadSheetWithTitle:@"Install Failed" message:@"There was an error installing Fountain Music."];
			}
			break;
		
		case 2:
			if ([self uninstallProduct]) {
				[self runHappySheetWithTitle:@"Removal Successful" 
									 message:@"Fountain Music has been uninstalled." 
							  restartMessage:@"Once iTunes is restarted, it will be fully removed."];
			} else {
				[self runSadSheetWithTitle:@"Removal Failed" message:@"There was an error removing Fountain Music from your system."];
			}
			
		default:
			break;
	}
}

- (BOOL)installProductToPath:(NSString *)path {
	// first uninstall all existing installations
	if (![self uninstallProduct]) return NO;
	
	NSString *sourcePath = [self productSourcePath];
	
	NSString *destPath = [[path stringByExpandingTildeInPath] stringByAppendingPathComponent:kProductFilename];
	
	NSFileManager *fm = [NSFileManager defaultManager];
	BOOL allGood = YES;
	
	NSError *err;
	if (![fm copyItemAtPath:sourcePath toPath:destPath error:&err]) {
		NSLog(@"Error installing to %@: %@", destPath, err);
		allGood = NO;
	}
	
	[self refreshProductIsInstalled];
	
	return allGood;
}

- (BOOL)uninstallProduct {
	NSFileManager *fm = [NSFileManager defaultManager];
	NSArray *paths = [self findExistingProductPaths];
	NSError *err;
	BOOL allGood = YES;
	
	for (NSString *path in paths) {
		if (![fm removeItemAtPath:path error:&err]) {
			NSLog(@"Error removing existing installation at %@: %@", path, err);
			allGood = NO;
		}
	}
	
	[self refreshProductIsInstalled];
	
	return allGood;
}

@end
