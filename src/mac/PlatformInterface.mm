#include "PlatformInterface.h"

#include "Utils.h"
#include "HumbugApplication.h"
#include "HumbugWindow.h"

#include <QDir>

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

class PlatformInterfacePrivate {
public:
    PlatformInterfacePrivate(PlatformInterface *qq) : q(qq) {
        QDir binDir(QApplication::applicationDirPath());
        binDir.cdUp();
        binDir.cd("Resources");

        const QString file = binDir.absoluteFilePath("humbug.wav");
        sound = [[NSSound alloc] initWithContentsOfFile:fromQString(file)
                                            byReference:NO];

    }

    ~PlatformInterfacePrivate() {
        [sound release];
    }

    NSSound *sound;
    PlatformInterface *q;
};

PlatformInterface::PlatformInterface(QObject *parent)
    : QObject(parent)
    , m_d(new PlatformInterfacePrivate(this))
{
    // Initialize Sparkle
    [[SUUpdater sharedUpdater] setDelegate: NSApp];

    [GrowlApplicationBridge setGrowlDelegate:[[ZGrowlDelegate alloc] init]];
}

PlatformInterface::~PlatformInterface() {
    delete m_d;
}

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

void PlatformInterface::playSound() {
    [m_d->sound play];
}
