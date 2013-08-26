#import "ZulipAppHelperDelegate.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

void setup() {
    BOOL alreadyRunning = NO, isActive = NO;
    NSArray *running = [[NSWorkspace sharedWorkspace] runningApplications];
    for (NSRunningApplication *app in running) {
        if ([[app bundleIdentifier] isEqualToString:@"com.zulip.Zulip"]) {
            alreadyRunning = YES;
            isActive = [app isActive];
        }
    }

    if (!alreadyRunning || !isActive) {
        NSString *path = [[NSBundle mainBundle] bundlePath];
        NSArray *p = [path pathComponents];
        NSMutableArray *pathComponents = [NSMutableArray arrayWithArray:p];
        [pathComponents removeLastObject];
        [pathComponents removeLastObject];
        [pathComponents removeLastObject];
        [pathComponents addObject:@"MacOS"];
        [pathComponents addObject:@"Zulip"];
        NSString *newPath = [NSString pathWithComponents:pathComponents];
        NSLog(@"Trying to start application: %@", newPath);
        [[NSWorkspace sharedWorkspace] launchApplication:newPath];
    }
}
