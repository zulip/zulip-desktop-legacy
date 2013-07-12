#include "ZulipApplication.h"

#include "ZulipWindow.h"
#include "webview/HWebView.h"
#include "mac/Converters.h"
#include "mac/Utils.h"
#include "Config.h"

#include <QDesktopWidget>

#import "Foundation/Foundation.h"

static inline NSPoint flipPoint(const NSPoint& loc) {
    return NSMakePoint(loc.x, QApplication::desktop()->screenGeometry(0).height() - loc.y);
}

// WebKit doesn't seem to be getting the mouseMove events properly when embedded in our app
// To work around this, we capture mousemove events ourselves, and if we see one that's in the
// webview rect, we send a notificationcenter event to the app.
bool ZulipApplication::macEventFilter(EventHandlerCallRef, EventRef event) {
    NSEvent *e = reinterpret_cast<NSEvent *>(event);

    if ([e type] == NSMouseMoved) {
        // If we're in the webview, pass it along
        const QPoint topLeft = m_mw->webView()->mapToGlobal(m_mw->webView()->geometry().topLeft());
        const QRect bounds(topLeft, m_mw->webView()->rect().size());
        NSPoint loc = flipPoint([[e window] convertBaseToScreen:[e locationInWindow]]);
        if (bounds.contains(loc.x, loc.y)) {
            [[NSNotificationCenter defaultCenter]
                  postNotificationName:@"NSMouseMovedNotification" object:nil
                  userInfo:[NSDictionary dictionaryWithObject:e forKey:@"NSEvent"]];
        }
    }
    return false;
}
