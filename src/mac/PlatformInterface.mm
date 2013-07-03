#include "PlatformInterface.h"

#include "Utils.h"
#include "HumbugApplication.h"
#include "HumbugWindow.h"

#import <Foundation/Foundation.h>
#import <Sparkle/SUUpdater.h>
#import <Growl/GrowlApplicationBridge.h>

@interface ZGrowlDelegate : NSObject <GrowlApplicationBridgeDelegate>
- (void) growlNotificationWasClicked:(id)clickContext;
@end

@implementation ZGrowlDelegate
- (void) growlNotificationWasClicked:(id)clickContext {
    APP->mainWindow()->trayClicked();
}
@end

PlatformInterface::PlatformInterface(QObject *parent)
    : QObject(parent)
    , m_d(0)
{
    // Initialize Sparkle
    [[SUUpdater sharedUpdater] setDelegate: NSApp];

    [GrowlApplicationBridge setGrowlDelegate:[[ZGrowlDelegate alloc] init]];
}

PlatformInterface::~PlatformInterface() {}

void PlatformInterface::checkForUpdates() {
    [[SUUpdater sharedUpdater] checkForUpdates: NSApp];
}

void PlatformInterface::desktopNotification(const QString &titleQ, const QString &contentQ) {
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

void PlatformInterface::unreadCountUpdated(int, int) {

}
