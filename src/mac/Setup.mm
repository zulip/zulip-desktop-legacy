#include "Setup.h"

#include "Config.h"
#include "Utils.h"
#include "HumbugApplication.h"
#include "HumbugWindow.h"

#ifdef HAVE_SPARKLE
    #import <Sparkle/SUUpdater.h>
#endif

#ifdef HAVE_GROWL
    #import <Growl/GrowlApplicationBridge.h>
#endif

@interface ZGrowlDelegate : NSObject <GrowlApplicationBridgeDelegate>
- (void) growlNotificationWasClicked:(id)clickContext;
@end

@implementation ZGrowlDelegate
- (void) growlNotificationWasClicked:(id)clickContext {
    APP->mainWindow()->trayClicked();
}
@end

void macMain() {
#ifdef HAVE_SPARKLE
    // Initialize Sparkle
    [[SUUpdater sharedUpdater] setDelegate: NSApp];
#endif

#ifdef HAVE_GROWL
    [GrowlApplicationBridge setGrowlDelegate:[[ZGrowlDelegate alloc] init]];

#endif
}

void macNotify(const QString &titleQ, const QString &contentQ) {
    // Bounce dock icon
    [NSApp requestUserAttention:NSCriticalRequest];

    // Show desktop notification
    NSString *title = fromQString(titleQ);
    NSString *content = fromQString(contentQ);

    [GrowlApplicationBridge notifyWithTitle:title description:content
                           notificationName:@"Message Notification"
                                   iconData:nil
                                   priority:0
                                   isSticky:NO
                               clickContext:nil];
}

void checkForSparkleUpdate() {
#ifdef HAVE_SPARKLE
  [[SUUpdater sharedUpdater] checkForUpdates: NSApp];
#endif
}
