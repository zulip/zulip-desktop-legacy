#include "HWebView.h"

#include "mac/Converters.h"
#include "mac/Utils.h"

#include <QMacCocoaViewContainer>
#include <QVBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QFile>

#import "Foundation/NSAutoreleasePool.h"
#import "Foundation/NSNotification.h"
#import "AppKit/NSApplication.h"
#import "Foundation/Foundation.h"
#import "WebKit/WebKit.h"
#import "AppKit/NSSearchField.h"

#define KEYCODE_A 0
#define KEYCODE_X 7
#define KEYCODE_C 8
#define KEYCODE_V 9
#define KEYCODE_Z 6

@interface HumbugWebDelegate : NSObject
{
    HWebViewPrivate* q;
}
// Init with our reference to our HWebViewPrivate
- (id)initWithPrivate:(HWebViewPrivate*)q;

// Handle clicked links
- (void)webView:(WebView *)webView decidePolicyForNewWindowAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request newFrameName:(NSString *)frameName decisionListener:(id < WebPolicyDecisionListener >)listener;

// Hook for adding our own JS window object
- (void)webView:(WebView *)sender didClearWindowObject:(WebScriptObject *)windowObject forFrame:(WebFrame *)frame;
+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector;
+ (BOOL)isKeyExcludedFromWebScript:(const char *)property;
+ (NSString *) webScriptNameForSelector:(SEL)sel;

// WebUIDelegate for handling open file click
- (void)webView:(WebView *)sender runOpenPanelForFileButtonWithResultListener:(id < WebOpenPanelResultListener >)resultListener;

// Methods we're sharing with JavaScript
- (void)updateCount:(int)newCount;
- (void)bell;
- (void)desktopNotification:(NSString*)title withContent:(NSString*)content;
@end

class HWebViewPrivate : public QObject {
    Q_OBJECT
public:
    HWebViewPrivate(HWebView* qq) : QObject(qq), q(qq), webView(0), delegate(0)
    {}
    ~HWebViewPrivate() {}

    void linkClicked(const QUrl& url) {
        emit q->linkClicked(url);
    }

    void updateCount(int newCount) {
        if (newCount > 0)
            [[NSApp dockTile] setBadgeLabel:[NSString stringWithFormat:@"%d", newCount]];
        else
            [[NSApp dockTile] setBadgeLabel:nil];

        emit q->updateCount(newCount);
    }

    void bell() {
        emit q->bell();
    }

    void desktopNotification(const QString& title, const QString& content) {
        emit q->desktopNotification(title, content);
    }

public:
    HWebView* q;
    WebView* webView;
    HumbugWebDelegate* delegate;
};

// Override performKeyEquivalent to make shortcuts work
@interface HumbugWebView : WebView
-(BOOL)performKeyEquivalent:(NSEvent*)event;
@end

@implementation HumbugWebView
-(BOOL)performKeyEquivalent:(NSEvent*)event {
    if ([event type] == NSKeyDown && [event modifierFlags] & NSCommandKeyMask)
    {
        const unsigned short keyCode = [event keyCode];
        if (keyCode == KEYCODE_A)
        {
            // NOTE selectAll is an undocumented API
            // see ./Source/WebKit/mac/WebView/WebView.mm:339 or thereabouts
            if ([self respondsToSelector:@selector(selectAll:)])
                [self selectAll:self];

            return YES;
        }
        else if (keyCode == KEYCODE_C)
        {
            [self copy:self];
            return YES;
        }
        else if (keyCode == KEYCODE_V)
        {
            [self paste:self];
            return YES;
        }
        else if (keyCode == KEYCODE_X)
        {
            [self cut:self];
            return YES;
        }
        else if (keyCode == KEYCODE_Z)
        {
            if ([event modifierFlags] & NSShiftKeyMask)
                [[self undoManager] redo];
            else
                [[self undoManager] undo];
            return YES;
        }
    }

    return NO;
}

@end

@implementation HumbugWebDelegate
- (id)initWithPrivate:(HWebViewPrivate*)qq {
    self = [super init];
    q = qq;

    return self;
}

- (void)webView:(WebView *)webView decidePolicyForNewWindowAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request newFrameName:(NSString *)frameName decisionListener:(id < WebPolicyDecisionListener >)listener {
    q->linkClicked(toQUrl([request URL]));

    [listener ignore];
}

- (void)webView:(WebView *)sender runOpenPanelForFileButtonWithResultListener:(id < WebOpenPanelResultListener >)resultListener {
    NSLog(@"Open File");

    // Open a file dialog, and save the result
    NSOpenPanel* fileDialog = [NSOpenPanel openPanel];
    [fileDialog setCanChooseFiles:YES];
    [fileDialog setCanChooseDirectories:NO];
    if ([fileDialog runModal] == NSOKButton)
    {
        NSArray* URLs = [fileDialog URLs];
        if ([URLs count] > 0) {
            [resultListener chooseFilename:[[URLs objectAtIndex:0] relativePath]];
        }
    }
}

- (void)webView:(WebView *)sender didClearWindowObject:(WebScriptObject *)windowObject forFrame:(WebFrame *)frame {
    NSLog(@"Setting up Cocoa<->WebScript bridge!");
    [windowObject setValue:self forKey:@"bridge"];
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector {
    if (selector == @selector(updateCount:) ||
        selector == @selector(bell) ||
        selector == @selector(desktopNotification:withContent:)) {
        return NO;
    }
    return YES;
}

+ (BOOL)isKeyExcludedFromWebScript:(const char *)property {
    // Don't expose any variables to javascript
    return YES;
}

+ (NSString *) webScriptNameForSelector:(SEL)sel {
    if (sel == @selector(updateCount:)) {
        return @"updateCount";
    } else if (sel == @selector(desktopNotification:withContent:)) {
        return @"desktopNotification";
    }
    return nil;
}

- (void)updateCount:(int)newCount {
    q->updateCount(newCount);
}

-(void)bell {
    q->bell();
}

- (void)desktopNotification:(NSString*)title withContent:(NSString*)content {
    q->desktopNotification(toQString(title), toQString(content));
}
@end

HWebView::HWebView(QWidget *parent)
    : QWidget(parent)
    , dptr(new HWebViewPrivate(this))
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    HumbugWebView *webView = [[HumbugWebView alloc] init];
    setupLayout(webView, this);

    WebPreferences *webPrefs = [webView preferences];

    // Cache as much as we can
    [webPrefs setCacheModel:WebCacheModelPrimaryWebBrowser];

    [webPrefs setPlugInsEnabled:YES];
    [webPrefs setUserStyleSheetEnabled:NO];

    // Try to enable LocalStorage
    if ([webPrefs respondsToSelector:@selector(_setLocalStorageDatabasePath:)])
        [webPrefs _setLocalStorageDatabasePath:@"~/Library/Application Support/Humbug/Humbug Desktop/LocalStorage"];
    if ([webPrefs respondsToSelector:@selector(setLocalStorageEnabled:)])
        [webPrefs setLocalStorageEnabled:YES];
    if ([webPrefs respondsToSelector:@selector(setDatabasesEnabled:)])
        [webPrefs setDatabasesEnabled:YES];
    [webView setPreferences:webPrefs];
    [webPrefs release];

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    dptr->webView = webView;

    dptr->delegate = [[HumbugWebDelegate alloc] initWithPrivate:dptr];
    [dptr->webView setPolicyDelegate:dptr->delegate];
    [dptr->webView setFrameLoadDelegate:dptr->delegate];
    [dptr->webView setUIDelegate:dptr->delegate];

    [webView release];

    [pool drain];
}

HWebView::~HWebView()
{
}

void HWebView::load(const QUrl &url) {
    if (url.scheme() == "qrc") {
        // Can't load qrc directly in Cocoa, need to tell it the HTML
        // QFile doesn't like the 'qrc` prefix.
        QString qrc = url.toString();
        qrc.replace("qrc:/", ":/");

        QFile f(qrc);
        if(!f.open(QIODevice::ReadOnly)) {
            NSLog(@"Unable to open qrc for reading, aborting qrc load: %@", fromQString(f.errorString()));
            return;
        }

        QByteArray data = f.readAll();
        NSString* content = fromQBA(data);
        [[dptr->webView mainFrame] loadHTMLString:content baseURL:[NSURL URLWithString:@"about:humbug"]];
        return;
    }

    NSURL* u = fromQUrl(url);

    [[dptr->webView mainFrame] loadRequest:[NSURLRequest requestWithURL:u]];
}

void HWebView::setUrl(const QUrl &url) {
    load(url);
}

QUrl HWebView::url() const {
    return QUrl();
}


#include "HWebView_mac.moc"
