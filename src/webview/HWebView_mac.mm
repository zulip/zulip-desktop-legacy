#include "HWebView.h"

#include "mac/Converters.h"
#include "mac/Utils_mac.h"
#include "mac/NSData+Base64.h"
#include "PlatformInterface.h"
#include "Config.h"
#include "ZulipApplication.h"
#include "Utils.h"

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

typedef void(^CSRFSnatcherCallback)(NSString *token);

@interface ZulipWebDelegate : NSObject
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
- (void)updatePMCount:(int)newCount;
- (void)bell;
- (void)desktopNotification:(NSString*)title withContent:(NSString*)content;
- (NSString *)desktopAppVersion;
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

    void updatePMCount(int newCount) {
        emit q->updatePMCount(newCount);
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
    ZulipWebDelegate* delegate;
};

// Override performKeyEquivalent to make shortcuts work
@interface ZulipWebView : WebView
{
    QWidget *qwidget;
}
-(void)setQWidget:(QWidget *)widget;
-(BOOL)performKeyEquivalent:(NSEvent*)event;

// For some reason on OS X 10.7 (it seems), Qt's mouse handler (qt_mac_handleTabletEvent in src/gui/kernel/qt_cocoa_helpers_mac.mm) has a pointer to this ZulipWebView and calls qt_qwidget on it. It expects the associated QWidget to be returned.
// I don't know why this ZulipWebView is there instead of the parent QCocoaView, but we work around it like this. Additionally, qt_clearQWidget is called if ZulipWebView responds to qt_qwidget, so we implement it to avoid a crash on exit.
-(QWidget *) qt_qwidget;
-(void)qt_clearQWidget;
@end

@implementation ZulipWebView
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
            // If the user is trying to paste an image, we grab it from her clipboard
            // and upload it through the webapp manually.
            // This is because Safari does not allow the JS to extract binary data (e.g. images)
            // from the clipboard.
            NSData *pngImage = [self pngInPasteboard];
            if (pngImage) {
                NSString *b64image = [[pngImage base64EncodedString] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
                NSString *uploadCommand = [NSString stringWithFormat:@""
                                           "var imageData = atob('%@');"
                                           "var uploadData = {type: 'image/png', data: imageData};"
                                           "$('#compose').trigger('imagedata-upload.zulip', uploadData);"
                                           , b64image];
                [self stringByEvaluatingJavaScriptFromString:uploadCommand];
            } else {
                [self paste:self];
            }
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

- (NSData *)pngInPasteboard
{
    NSData *image = nil;
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *items = [pasteboard pasteboardItems];
    for (NSPasteboardItem *item in items) {
        NSArray *types = @[NSPasteboardTypeTIFF, NSPasteboardTypePNG];
        NSString *imageMimeTypeFound = [item availableTypeFromArray:types];
        if (imageMimeTypeFound) {
            // Got an image in the pasteboard, lets use it
            image = [item dataForType:imageMimeTypeFound];
        }
    }

    if (image) {
        // We get a TIFF or PNG, but want a PNG, so convert it regardless
        NSBitmapImageRep *rep = [NSBitmapImageRep imageRepWithData:image];
        return [rep representationUsingType:NSPNGFileType properties:nil];
    }
    return nil;
}

-(QWidget *) qt_qwidget {
    return qwidget;
}

-(void)setQWidget:(QWidget *)widget {
    qwidget = widget;
}

-(void)qt_clearQWidget {
    // This is called
}

@end

@interface ZulipCookieSnatcher : NSObject

@property (nonatomic, retain) NSString *siteURL;
@property (readwrite, copy) CSRFSnatcherCallback callback;

- (id)initWithSite:(NSString *)siteURL callback:(CSRFSnatcherCallback)callback;

@end

@implementation ZulipCookieSnatcher

- (id)initWithSite:(NSString *)siteURL callback:(CSRFSnatcherCallback)callback
{
    self = [super init];
    if (self) {
        self.siteURL = siteURL;
        self.callback = callback;
    }
    return self;
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame {
    NSHTTPCookieStorage *cookies = [NSHTTPCookieStorage sharedHTTPCookieStorage];
    NSArray *cookieList = [cookies cookiesForURL:[NSURL URLWithString:self.siteURL]];
    for (NSHTTPCookie *cookie in cookieList) {
        if ([[cookie name] isEqualToString:@"csrftoken"]) {
            self.callback([cookie value]);
            return;
        }
    }

    self.callback(nil);
}

@end


@implementation ZulipWebDelegate
- (id)initWithPrivate:(HWebViewPrivate*)qq {
    self = [super init];
    q = qq;

    return self;
}

- (void)csrfTokenForZulipServer:(NSString *)siteUrl callback:(CSRFSnatcherCallback)callback
{
    WebView *wv = [[WebView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
    [wv setFrameLoadDelegate:[[ZulipCookieSnatcher alloc] initWithSite:siteUrl callback:callback]];

    // Load the login page directly to avoid a redirect
    [wv setMainFrameURL:[NSString stringWithFormat:@"%@%@", siteUrl, @"login/?next=/"]];
}

- (NSURLRequest *)webView:(WebView *)sender resource:(id)identifier willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse fromDataSource:(WebDataSource *)dataSource {

    if (!APP->explicitDomain() &&
        [[request HTTPMethod] isEqualToString:@"POST"] &&
        [[[request URL] path] isEqualToString:@"/accounts/login"])
    {
        NSString *stringBody = [[NSString alloc] initWithData:[request HTTPBody] encoding:NSUTF8StringEncoding];
        __block QHash<QString, QString> paramDict = Utils::parseURLParameters(toQString(stringBody));
        NSString *email = fromQString(paramDict.value("username", QString()));

        if ([email length] == 0) {
            return request;
        }

        NSLog(@"Preflighting login request for %@", email);
        // Do a GET to the central Zulip server to determine the domain for this user
        NSString *preflightURL = [NSString stringWithFormat:@"https://api.zulip.com/v1/deployments/endpoints?email=%@", email];
        NSURLRequest *req = [NSURLRequest requestWithURL:[NSURL URLWithString:preflightURL]];

        NSURLResponse *response = nil;
        NSError *error = nil;
        NSData *responseData = [NSURLConnection sendSynchronousRequest:req returningResponse:&response error:&error];
        if (error) {
            NSLog(@"Error making preflight request: %@ %@", [error localizedDescription], [error userInfo]);
            return request;
        }

        error = nil;
        NSDictionary *json = [NSJSONSerialization JSONObjectWithData:responseData options:0 error:&error];
        if (error) {
            NSLog(@"Error decoding JSON: %@ %@", [error localizedDescription], [error userInfo]);
            return request;
        }
        NSDictionary *result = [json objectForKey:@"result"];
        if (!result) {
            return request;
        }

        __block NSString *baseSiteURL = [result objectForKey:@"base_site_url"];
        NSURL *siteURL = [NSURL URLWithString:baseSiteURL];
        NSString *siteHost = [siteURL host];

        // We have the "true" base URL to use
        // If it's the same as what we're already on---just let the existing request go through
        NSString *requestHost = [[request URL] host];
        if ([siteHost isEqualToString:requestHost]) {
            return request;
        }

        APP->setExplicitDomain(toQString(baseSiteURL));
        // However if it's different, we need to do the redirect
        // First, we need a valid CSRF token for our login form
        [self csrfTokenForZulipServer:baseSiteURL callback:^(NSString *token) {
            if (token == nil) {
                // We failed to get a csrf token, so aborting!
                NSLog(@"ERROR: Failed to grab a CSRF token from %@", baseSiteURL);
                return;
            }

            // If our base site URL ends with a trailing /, our path also starts with a /
            // so remove one of them
            if ([baseSiteURL characterAtIndex:[baseSiteURL length] - 1] == '/') {
                baseSiteURL = [baseSiteURL substringToIndex:[baseSiteURL length] - 1];
            }

            NSString *newURLString = [NSString stringWithFormat:@"%@%@/?%@", baseSiteURL, [[request URL] path], [[request URL] query]];
            NSURL *newURL = [NSURL URLWithString:newURLString];

            // NSURL is immutable so we have to construct a new one with the modified host
            NSMutableURLRequest *newRequest = [request mutableCopy];
            [newRequest setURL:newURL];
            // Spoof the Origin and Referer headers
            [newRequest setValue:baseSiteURL forHTTPHeaderField:@"Origin"];
            [newRequest setValue:[NSString stringWithFormat:@"%@/login/", baseSiteURL] forHTTPHeaderField:@"Referer"];

            // Splice in our new CSRF token into the POST body
            paramDict["csrfmiddlewaretoken"] = toQString(token);
            NSString *newBody = fromQString(Utils::parametersDictToString(paramDict));
            [newRequest setHTTPBody:[newBody dataUsingEncoding:NSUTF8StringEncoding]];

            // Make our new request in the web frame---this will cause a redirect to the logged-in
            // Zulip at the target site
            [[sender mainFrame] loadRequest:newRequest];
        }];

        // While our async CSRF-snatcher is working, we need to return a valid NSURLRequest, but we don't
        // want the user to be moved to any new page
        return [NSURLRequest requestWithURL:[NSURL URLWithString:@""]];
    }
    if ([[request HTTPMethod] isEqualToString:@"POST"] &&
        [[[request URL] path] isEqualToString:@"/accounts/logout"]) {
        // On logout, reset our "custom domain" if we saved one
        APP->setExplicitDomain(QString());
    }
    return request;
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
    [windowObject setValue:self forKey:@"bridge"];

    // The built-in WebKit sendAsBinary crashes when we try to use it with a large
    // payload---specifically, when pasting large imagines. The traceback is deep in
    // array handling code in JSC.
    //
    // To avoid the crash, we replace the native sendAsBinary with the suggested polyfill
    // using typed arrays, from https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest#sendAsBinary()
    [sender stringByEvaluatingJavaScriptFromString:
     @"XMLHttpRequest.prototype.sendAsBinary = function (sData) { "
             "var nBytes = sData.length, ui8Data = new Uint8Array(nBytes); "
             "for (var nIdx = 0; nIdx < nBytes; nIdx++) { "
                 "ui8Data[nIdx] = sData.charCodeAt(nIdx) & 0xff; "
             "} "
             "this.send(ui8Data.buffer); "
           "}"
     ];
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector {
    if (selector == @selector(updateCount:) ||
        selector == @selector(updatePMCount:) ||
        selector == @selector(bell) ||
        selector == @selector(desktopNotification:withContent:) ||
        selector == @selector(desktopAppVersion)) {
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
    } else if (sel == @selector(updatePMCount:)) {
        return @"updatePMCount";
    } else if (sel == @selector(desktopNotification:withContent:)) {
        return @"desktopNotification";
    } else if (sel == @selector(desktopAppVersion)) {
        return @"desktopAppVersion";
    }
    return nil;
}

- (void)updateCount:(int)newCount {
    q->updateCount(newCount);
}

- (void)updatePMCount:(int)newCount {
    q->updatePMCount(newCount);
}

-(void)bell {
    q->bell();
}

- (void)desktopNotification:(NSString*)title withContent:(NSString*)content {
    q->desktopNotification(toQString(title), toQString(content));
}

- (NSString *)desktopAppVersion {
    return fromQString(PlatformInterface::platformWithVersion());
}

@end

HWebView::HWebView(QWidget *parent)
    : QWidget(parent)
    , dptr(new HWebViewPrivate(this))
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    ZulipWebView *webView = [[ZulipWebView alloc] init];
    [webView setQWidget:this];
    setupLayout(webView, this);

    [webView setApplicationNameForUserAgent:[NSString stringWithFormat:@"Zulip Desktop/%@", fromQString(ZULIP_VERSION_STRING)]];

    WebPreferences *webPrefs = [webView preferences];

    // Cache as much as we can
    [webPrefs setCacheModel:WebCacheModelPrimaryWebBrowser];

    [webPrefs setPlugInsEnabled:YES];
    [webPrefs setUserStyleSheetEnabled:NO];

    // Try to enable LocalStorage
    if ([webPrefs respondsToSelector:@selector(_setLocalStorageDatabasePath:)])
        [webPrefs performSelector:@selector(_setLocalStorageDatabasePath:) withObject:@"~/Library/Application Support/Zulip/Zulip Desktop/LocalStorage"];
    if ([webPrefs respondsToSelector:@selector(setLocalStorageEnabled:)])
        [webPrefs performSelector:@selector(setLocalStorageEnabled:) withObject:@(YES)];
    if ([webPrefs respondsToSelector:@selector(setDatabasesEnabled:)])
        [webPrefs performSelector:@selector(setDatabasesEnabled:) withObject:@(YES)];
    [webView setPreferences:webPrefs];
    [webPrefs release];

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    dptr->webView = webView;

    dptr->delegate = [[ZulipWebDelegate alloc] initWithPrivate:dptr];
    [dptr->webView setPolicyDelegate:dptr->delegate];
    [dptr->webView setResourceLoadDelegate:dptr->delegate];
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
        [[dptr->webView mainFrame] loadHTMLString:content baseURL:[NSURL URLWithString:@"about:zulip"]];
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

void HWebView::loadHTML(const QString &html) {
    NSString* content = fromQString(html);
    [[dptr->webView mainFrame] loadHTMLString:content baseURL:[NSURL URLWithString:@"about:zulip"]];
}


#include "HWebView_mac.moc"
