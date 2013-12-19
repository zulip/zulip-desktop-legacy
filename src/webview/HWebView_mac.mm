#include "HWebView.h"

#include "mac/Converters.h"
#include "mac/Utils_mac.h"
#include "mac/NSData+Base64.h"
#include "PlatformInterface.h"
#include "Config.h"
#include "ZulipApplication.h"
#include "Utils.h"
#include "mac/NSArray+Blocks.h"

#include <QBuffer>
#include <QDir>
#include <QDesktopServices>
#include <QMacCocoaViewContainer>
#include <QVBoxLayout>
#include <QApplication>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QClipboard>
#include <QFile>

#import "thirdparty/BSHTTPCookieStorage/BSHTTPCookieStorage.h"

#import "AppKit/NSApplication.h"
#import "WebKit/WebKit.h"
#import "AppKit/NSSearchField.h"

#if defined(QT5_BUILD)
#include <QJsonDocument>
#elif defined(QT4_BUILD)
#include "qjsondocument.h"
#endif

#define KEYCODE_A 0
#define KEYCODE_X 7
#define KEYCODE_C 8
#define KEYCODE_V 9
#define KEYCODE_Z 6

typedef void(^CSRFSnatcherCallback)(NSString *token);

// QWebkit, WHY DO YOU MAKE ME DO THIS
typedef QHash<QNetworkReply *, QByteArray *> PayloadHash;
Q_GLOBAL_STATIC(PayloadHash, s_imageUploadPayloads);

#pragma mark - InitialRequest and AskForDomain Callbacks

typedef void(^ConnectionStatusCallback)(Utils::ConnectionStatus status);
typedef void(^AskForDomainSuccess)(QString domain);
typedef void(^AskForDomainRetry)();

class ConnectionStatusCallbackWatcher : public QObject {
    Q_OBJECT
public:
    ConnectionStatusCallbackWatcher(ConnectionStatusCallback callback)
        : QObject(0),
          m_callback(callback)
    {

    }

public Q_SLOTS:
    void connectionStatusSlot(Utils::ConnectionStatus status) {
        m_callback(status);
        deleteLater();
    }
private:
    ConnectionStatusCallback m_callback;
};

class AskForDomainCallbackWatcher : public QObject {
    Q_OBJECT
public:
    AskForDomainCallbackWatcher(AskForDomainSuccess success, AskForDomainRetry retry)
    : QObject(0),
      m_success(success),
      m_retry(retry)
    {}

public Q_SLOTS:
    void success(const QString& domain) {
        m_success(domain);
        deleteLater();
    }

    void retry() {
        m_retry();
        deleteLater();
    }
private:
    AskForDomainSuccess m_success;
    AskForDomainRetry m_retry;
};

#pragma mark - ZulipWebDelegate Interface

@interface ZulipWebDelegate : NSObject
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

// Standalone cookie options
- (void)webView:(WebView *)sender resource:(id)identifier didReceiveResponse:(NSURLResponse *)response fromDataSource:(WebDataSource *)dataSource;
- (void)saveCookies;
- (void)loadCookies;
- (void)migrateSystemCookies;

// Methods we're sharing with JavaScript
- (void)updateCount:(int)newCount;
- (void)updatePMCount:(int)newCount;
- (void)bell;
- (void)desktopNotification:(NSString*)title withContent:(NSString*)content;
- (NSString *)desktopAppVersion;
- (void)log:(NSString *)msg;
- (void)websocketPreOpen;
- (void)websocketPostOpen;

@property (nonatomic) HWebViewPrivate *q;

// For CSRF/Auto-redirecting
@property (nonatomic, retain)NSURLRequest *origRequest;

// Isolated cookies
@property (nonatomic, retain)BSHTTPCookieStorage *cookieStorage;
@property (nonatomic, copy)NSDate *cookieFileSaveDate;

@property (nonatomic, retain) NSArray *systemCookies;
@property (nonatomic, assign) BOOL storedSystemCookies;
@end

#pragma mark - HWebViewPrivate

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

public slots:
    void imageUploadFinished() {
        QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        if (s_imageUploadPayloads()->contains(reply)) {
            delete s_imageUploadPayloads()->take(reply);
        }

        const QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (reply->error() == QNetworkReply::NoError && error.error == QJsonParseError::NoError) {
            QVariantMap result = doc.toVariant().toMap();
            const QString imageLink = result.value("uri", QString()).toString();

            [webView stringByEvaluatingJavaScriptFromString:[NSString stringWithFormat:@"compose.uploadFinished(0, undefined, {\"uri\": \"%@\"}, undefined);", fromQString(imageLink)]];

            return;
        } else {
            qDebug() << "Image Upload Error:" << reply->errorString() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << data;
        }
        [webView stringByEvaluatingJavaScriptFromString:@"compose.uploadError('Unknown Error');"];
    }

    void imageUploadProgress(qint64 bytesSent ,qint64 bytesTotal) {
        // Trigger progress bar change
        if (bytesTotal > 0) {
            int percent = (bytesSent * 100 / bytesTotal);
            [webView stringByEvaluatingJavaScriptFromString:[NSString stringWithFormat:@"compose.progressUpdated(0, undefined, \"%d\");", percent]];
        }
    }

    void imageUploadError(QNetworkReply::NetworkError error) {
        qDebug() << "Got Upload error:" << error;
        [webView stringByEvaluatingJavaScriptFromString:@"compose.uploadError('Unknown Error');"];
    }

public:
    HWebView* q;
    WebView* webView;
    ZulipWebDelegate* delegate;
    QUrl originalURL;
};

#pragma mark - ZulipWebView

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
            // OS X 10.7's WebKit version does not support image uploading, so
            // we upload manually
            // NSAppKitVersionNumber10_8 is not defined on our Lion build machine
            // since it has too old of an SDK, so we hardcode the version here:
            // NSApplication.h:44 #define NSAppKitVersionNumber10_8 1187
            if (NSAppKitVersionNumber < 1187) {
                qDebug() << "Uploading via QHttpMimeData";
                const bool isImage = [self uploadImageWithQt];
                if (!isImage) {
                    [self paste:self];
                }
            } else {
                qDebug() << "Uploading via WebKit";

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

- (bool)uploadImageWithQt
{
    NSData *pngImage = [self pngInPasteboard];
    if (!pngImage) {
        return false;
    }

    // We need to get a QBuffer that contains the image data
    // QBuffers are wrappers around a QByteArray, so take our
    // image from the NSData* and stick it in a QByteArray
    QByteArray *data = new QByteArray;
    data->resize([pngImage length]);
    [pngImage getBytes:data->data() length:[pngImage length]];
    QBuffer *buffer = new QBuffer(data, 0);
    buffer->open(QIODevice::ReadOnly);

    // Get CSRF token and Session ID
    ZulipWebDelegate *delegate = (ZulipWebDelegate *)[self frameLoadDelegate];
    NSArray *cookieList = [delegate.cookieStorage cookiesForURL:fromQUrl(delegate.q->originalURL)];
    QString csrfToken, sessionID;
    for (NSHTTPCookie *cookie in cookieList) {
        if ([[cookie name] isEqualToString:@"csrftoken"]) {
            csrfToken = toQString([cookie value]);
        } else if ([[cookie name] isEqualToString:@"sessionid"]) {
            sessionID = toQString([cookie value]);
        }
    }

    // Populate a QNAM with the csrftoken and sessionid cookies
    QNetworkAccessManager *nam = new QNetworkAccessManager;

    const QString qUrlString = delegate.q->originalURL.toString();
    QNetworkCookieJar *jar = nam->cookieJar();
    QNetworkCookie csrfCookie("csrftoken", csrfToken.toUtf8());
    jar->setCookiesFromUrl(QList<QNetworkCookie>() << QNetworkCookie("csrftoken", csrfToken.toUtf8())
                           << QNetworkCookie("sessionid", sessionID.toUtf8()),
                           QUrl(qUrlString));

    // Begin the image upload
    QNetworkReply *imageUpload = Utils::uploadImageFromBuffer(buffer, csrfToken, qUrlString, nam);

    // Delete our created NAM when the reply is deleted
    QObject::connect(imageUpload, SIGNAL(destroyed(QObject*)), nam, SLOT(deleteLater()));

    QObject::connect(imageUpload, SIGNAL(finished()), delegate.q, SLOT(imageUploadFinished()));
    QObject::connect(imageUpload, SIGNAL(uploadProgress(qint64,qint64)), delegate.q, SLOT(imageUploadProgress(qint64, qint64)));
    QObject::connect(imageUpload, SIGNAL(error(QNetworkReply::NetworkError)), delegate.q, SLOT(imageUploadError(QNetworkReply::NetworkError)));

    s_imageUploadPayloads()->insert(imageUpload, data);

    [self stringByEvaluatingJavaScriptFromString:@"compose.uploadStarted()"];

    return true;
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

#pragma mark - ZulipCookieSnatcher

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
    ZulipWebDelegate *delegate = (ZulipWebDelegate *)[sender frameLoadDelegate];
    NSArray *cookieList = [delegate.cookieStorage cookiesForURL:[NSURL URLWithString:self.siteURL]];
    for (NSHTTPCookie *cookie in cookieList) {
        if ([[cookie name] isEqualToString:@"csrftoken"]) {
            self.callback([cookie value]);
            return;
        }
    }

    self.callback(nil);
}

@end

#pragma mark - ZulipWebDelegate

@implementation ZulipWebDelegate
- (id)initWithPrivate:(HWebViewPrivate*)qq {
    self = [super init];

    if (self) {
        self.origRequest = nil;
        self.cookieStorage = [[BSHTTPCookieStorage alloc] init];
        self.cookieFileSaveDate = nil;
        self.systemCookies = @[];
        self.storedSystemCookies = NO;
        self.q = qq;

        [self loadCookies];
    }
    return self;
}

- (void)dealloc
{
    // In case we are quitting while we've swizzled
    [self restoreSystemCookies];
}

- (void)csrfTokenForZulipServer:(NSString *)siteUrl callback:(CSRFSnatcherCallback)callback
{
    WebView *wv = [[WebView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
    [wv setFrameLoadDelegate:[[ZulipCookieSnatcher alloc] initWithSite:siteUrl callback:callback]];

    // Load the login page directly to avoid a redirect
    [wv setMainFrameURL:[NSString stringWithFormat:@"%@%@", siteUrl, @"login/?next=/"]];
}

- (bool)snatchCSRFAndRedirect:(NSString *)customDomain {
    NSURLRequest *request = self.origRequest;

    __block NSString *baseSiteURL = customDomain;

    if ([baseSiteURL characterAtIndex:[baseSiteURL length] - 1] != '/') {
        baseSiteURL = [NSString stringWithFormat:@"%@/", baseSiteURL];
    }

    NSURL *siteURL = [NSURL URLWithString:baseSiteURL];
    NSString *siteHost = [siteURL host];

    // We have the "true" base URL to use
    // If it's the same as what we're already on---just let the existing request go through
    NSString *requestHost = [[request URL] host];
    if ([siteHost isEqualToString:requestHost]) {
        return false;
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
        NSString *stringBody = [[NSString alloc] initWithData:[request HTTPBody] encoding:NSUTF8StringEncoding];
        QHash<QString, QString> paramDict = Utils::parseURLParameters(toQString(stringBody));
        paramDict["csrfmiddlewaretoken"] = toQString(token);
        NSString *newBody = fromQString(Utils::parametersDictToString(paramDict));
        [newRequest setHTTPBody:[newBody dataUsingEncoding:NSUTF8StringEncoding]];

        // Make our new request in the web frame---this will cause a redirect to the logged-in
        // Zulip at the target site
        [[self.q->webView mainFrame] loadRequest:newRequest];
    }];

    return true;
}

- (NSURLRequest *)webView:(WebView *)sender resource:(id)identifier willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse fromDataSource:(WebDataSource *)dataSource
{
    if ([[request HTTPMethod] isEqualToString:@"POST"] &&
        [[[request URL] path] isEqualToString:@"/accounts/logout"]) {
        // On logout, reset our "custom domain" if we saved one
        APP->setExplicitDomain(QString());
    }

    if ([redirectResponse isKindOfClass:[NSHTTPURLResponse class]]) {
        [self.cookieStorage handleCookiesInResponse:(NSHTTPURLResponse*) redirectResponse];
        [self saveCookies];
    }

    NSMutableURLRequest* modifiedRequest = [request mutableCopy];
    [modifiedRequest setHTTPShouldHandleCookies:NO];
    [modifiedRequest setValue:fromQString(PlatformInterface::userAgentString()) forHTTPHeaderField:@"User-Agent"];
    [self.cookieStorage handleCookiesInRequest:modifiedRequest];

    // Disable automatic redirection for local server until bugs are ironed out
    return modifiedRequest;

    if (!APP->explicitDomain() &&
        [[request HTTPMethod] isEqualToString:@"POST"] &&
        [[[request URL] path] isEqualToString:@"/accounts/login"])
    {
        NSString *stringBody = [[NSString alloc] initWithData:[request HTTPBody] encoding:NSUTF8StringEncoding];
        QHash<QString, QString> paramDict = Utils::parseURLParameters(toQString(stringBody));
        NSString *email = fromQString(paramDict.value("username", QString()));

        if ([email length] == 0) {
            return request;
        }

        NSURLRequest *emptyRequest = [NSURLRequest requestWithURL:[NSURL URLWithString:@""]];
        self.origRequest = request;

        NSLog(@"Preflighting login request for %@", email);
        // Do a GET to the central Zulip server to determine the domain for this user
        bool requestSuccessful;
        NSString *preflightURL = fromQString(Utils::baseUrlForEmail(0, toQString(email), &requestSuccessful));

        AskForDomainCallbackWatcher *callback = new AskForDomainCallbackWatcher(^(QString domain) {
            [self snatchCSRFAndRedirect:fromQString(domain)];
        }, ^{
            // Retry
            // Make a strong reference to the parameters
            // so they are still around when our delayed block gets executed
            __block WebView* _sender = sender;
            __block id _identifier = identifier;
            __block NSURLRequest *_request = request;
            __block NSURLResponse *_redirectResponse = redirectResponse;
            __block WebDataSource *_dataSource = dataSource;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0), dispatch_get_main_queue(), ^(void){
                [self webView:_sender resource:_identifier willSendRequest:_request redirectResponse:_redirectResponse fromDataSource:_dataSource];
            });
        });

        if (!requestSuccessful) {
            NSLog(@"Error making preflight request, asking");
            APP->askForCustomServer(callback);
            return emptyRequest;
        } else if ([preflightURL isEqualToString:@""]) {
            return request;
        }

        if (![self snatchCSRFAndRedirect:preflightURL]) {
            return request;
        } else {
            // While our async CSRF-snatcher is working, we need to return a valid NSURLRequest, but we don't
            // want the user to be moved to any new page
            return emptyRequest;
        }
    }
    if ([[request HTTPMethod] isEqualToString:@"POST"] &&
        [[[request URL] path] isEqualToString:@"/accounts/logout"]) {
        // On logout, reset our "custom domain" if we saved one
        APP->setExplicitDomain(QString());
    }
    return request;
}

- (void)webView:(WebView *)sender resource:(id)identifier didReceiveResponse:(NSURLResponse *)response fromDataSource:(WebDataSource *)dataSource
{
    if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
        [self.cookieStorage handleCookiesInResponse:(NSHTTPURLResponse*) response];
        [self saveCookies];
    }
}

- (void)webView:(WebView *)sender didReceiveServerRedirectForProvisionalLoadForFrame:(WebFrame *)frame
{
    NSString *orig_url = [[[[frame dataSource] request] URL] absoluteString];
    NSString *redirected_url = [[[[frame provisionalDataSource] request] URL] absoluteString];

    // We capture the redirect from the deployment_dispatch SSO login page, as this is when
    // our Zulip server is telling is where this particular user's SSO deployment is hosted
    if ([orig_url rangeOfString:@"accounts/deployment_dispatch"].location != NSNotFound) {
        const QString domain = toQString(redirected_url);
        qDebug() << "Got redirect from deployment_dispatch login, saving as explicit domain:" << domain;
        APP->setExplicitDomain(domain);
    }
}

- (void)webView:(WebView *)webView decidePolicyForNewWindowAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request newFrameName:(NSString *)frameName decisionListener:(id < WebPolicyDecisionListener >)listener {
    self.q->linkClicked(toQUrl([request URL]));

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

    QFile extraJS(":/mac/JSExtras.js");
    if (!extraJS.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to read from :/mac/JSExtras.js";
    } else {
        NSString *contents = fromQString(QString::fromUtf8(extraJS.readAll()));
        [sender stringByEvaluatingJavaScriptFromString:contents];
    }
}

// Grab console.log output to our own logfile
// This is an undocumented delegate method found in WebUIDelegatePrivate.h
- (void)webView:(WebView *)webView addMessageToConsole:(NSDictionary *)message withSource:(NSString *)source
{
    if (![message isKindOfClass:[NSDictionary class]]) {
        return;
    }

    NSString *messageString = [message objectForKey:@"message"];
    NSString *messageLevel = [message objectForKey:@"MessageLevel"];
    NSString *sourceURL = [message objectForKey:@"sourceURL"];
    int columnNumber = [[message objectForKey:@"columnNumber"] intValue];
    int lineNumber = [[message objectForKey:@"lineNumber"] intValue];

    // Use qDebug() as it ties in to our log handler
    qDebug() << "WebKit Console Message" << toQString(messageString) << "from" << toQString(messageLevel)
             << toQString(sourceURL) << "line" << lineNumber << "col" << columnNumber;
}

- (NSArray *)webView:(WebView *)sender contextMenuItemsForElement:(NSDictionary *)element defaultMenuItems:(NSArray *)defaultMenuItems
{
    NSArray *filteredActionNames = @[@"Open Link", @"Open Link in New Window", @"Download Linked File"];
    NSArray *filteredMenuItems = [defaultMenuItems filter:^BOOL(id obj) {
        return  [filteredActionNames indexOfObject:[obj title]] == NSNotFound;
    }];
    return filteredMenuItems;
}

- (NSString *)cookiesFilePath
{
    QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    if (!data_dir.exists()) {
        data_dir.mkpath(".");
    }
    return fromQString(data_dir.absoluteFilePath("Zulip_Cookies.dat"));
}

- (void)saveCookies
{
    // De-bounce saves to every 3s
    if (!self.cookieFileSaveDate ||
        [[NSDate date] timeIntervalSinceDate:self.cookieFileSaveDate] > 3.0)
    {
        NSString *filePath = [self cookiesFilePath];
        NSData *data = [NSKeyedArchiver archivedDataWithRootObject:self.cookieStorage];
        [data writeToFile:filePath atomically:YES];

        self.cookieFileSaveDate = [NSDate date];
    }
}

- (void)loadCookies
{
    NSString *filePath = [self cookiesFilePath];

    if (![[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
        [self migrateSystemCookies];
        return;
    }

    NSData *data = [[NSFileManager defaultManager] contentsAtPath:filePath];
    self.cookieStorage = [NSKeyedUnarchiver unarchiveObjectWithData:data];
}

- (void)migrateSystemCookies
{
    // One-off migration of cookies for zulip from system cookie jar
    // to avoid logging user out on update
    QSettings s;
    bool alreadyImportedFromSystem = s.value("ImportedFromSystem", false).toBool();
    if (!alreadyImportedFromSystem) {
        QString domain = s.value("Domain", "http://zulip.com").toString();
        NSURL *url = [NSURL URLWithString:fromQString(domain)];
        qDebug() << "Migrating system cookies on initial Zulip upgrade! (for " << domain << ")";
        NSArray *systemCookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:url];
        for (NSHTTPCookie *cookie in systemCookies) {
            [self.cookieStorage setCookie:cookie];
        }
        s.setValue("ImportedFromSystem", true);
    }
}

- (void)handleInitialLoadFailed {
    // If we failed to load Zulip **and** we don't have an explicit domain set,
    // prompt for a custom domain. An intranet with a local Zulip install might block
    // the zulip.com default page load

    if (APP->explicitDomain() || self.origRequest) {
        return;
    }

    ConnectionStatusCallbackWatcher *watcher = new ConnectionStatusCallbackWatcher(^(Utils::ConnectionStatus status) {
        if (status != Utils::Online) {
            [self askForInitialLoadDomain];
        }
    });

    Utils::connectedToInternet(0, watcher);
}

- (void)askForInitialLoadDomain {
    NSLog(@"Failed to initially load Zulip (explicit domain %i and origRequest %@", APP->explicitDomain(), self.origRequest);
    AskForDomainCallbackWatcher *callback = new AskForDomainCallbackWatcher(^(QString domain) {
        // We don't do anything fancy with the proper domain---we just load
        // it in the webview directly
        [[self.q->webView mainFrame] loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:fromQString(domain)]]];
    }, ^{
        // Retry
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0), dispatch_get_main_queue(), ^(void){
            // For some reason calling reload: or reloadFromOrigin both don't cause a reload,
            // yet calling loadRequest does.
            [[self.q->webView mainFrame] loadRequest:[NSURLRequest requestWithURL:fromQUrl(self.q->originalURL)]];
        });
    });
    APP->askForCustomServer(callback);
}

- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame {
    // Disable local-server firewall guess
//    [self handleInitialLoadFailed];
}

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame {
    // Disable local-server firewall guess
//    [self handleInitialLoadFailed];
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector {
    if (selector == @selector(updateCount:) ||
        selector == @selector(updatePMCount:) ||
        selector == @selector(bell) ||
        selector == @selector(desktopNotification:withContent:) ||
        selector == @selector(desktopAppVersion) ||
        selector == @selector(log:) ||
        selector == @selector(websocketPreOpen) ||
        selector == @selector(websocketPostOpen)) {
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
    } else if (sel == @selector(log:)) {
        return @"log";
    } else if (sel == @selector(websocketPostOpen)) {
        return @"websocketPostOpen";
    } else if (sel == @selector(websocketPreOpen)) {
        return @"websocketPreOpen";
    }
    return nil;
}

- (void)updateCount:(int)newCount {
    self.q->updateCount(newCount);
}

- (void)updatePMCount:(int)newCount {
    self.q->updatePMCount(newCount);
}

-(void)bell {
    self.q->bell();
}

- (void)desktopNotification:(NSString*)title withContent:(NSString*)content {
    self.q->desktopNotification(toQString(title), toQString(content));
}

- (NSString *)desktopAppVersion {
    return fromQString(PlatformInterface::platformWithVersion());
}

- (void)log:(NSString *)msg {
    // Use qDebug() for file-logging and timestamps
    qDebug() << "WebView Log:" << toQString(msg);
}

- (void)websocketPreOpen
{
    [self injectOurCookies];
}

- (void)websocketPostOpen
{
    [self restoreSystemCookies];
}

// Replace the system cookie jar with our own cookies
// just for long enough that the HTTP request to initiate
// the WebSocket protocol goes through
- (void)injectOurCookies
{
    NSURL *url = [[[[self.q->webView mainFrame] dataSource] request] URL];
    self.systemCookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:url];
    for (NSHTTPCookie *cookie in self.systemCookies) {
        [[NSHTTPCookieStorage sharedHTTPCookieStorage] deleteCookie:cookie];
    }

    self.storedSystemCookies = YES;
    NSArray *cookies = [self.cookieStorage cookiesForURL:url];
    [[NSHTTPCookieStorage sharedHTTPCookieStorage] setCookies:cookies forURL:url mainDocumentURL:nil];

    // Timeout in 5s to be safe so we don't leave our cookies hanging
    double delayInSeconds = 5.0;
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
    dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
        [self restoreSystemCookies];
    });
}

- (void)restoreSystemCookies
{
    if (!self.storedSystemCookies) {
        return;
    }
    self.storedSystemCookies = NO;

    NSURL *url = [[[[self.q->webView mainFrame] dataSource] request] URL];
    for (NSHTTPCookie *cookie in [self.cookieStorage cookiesForURL:url]) {
        [[NSHTTPCookieStorage sharedHTTPCookieStorage] deleteCookie:cookie];
    }

    [[NSHTTPCookieStorage sharedHTTPCookieStorage] setCookies:self.systemCookies forURL:url mainDocumentURL:nil];
    self.systemCookies = @[];
}

@end

#pragma mark - HWebView

HWebView::HWebView(QWidget *parent)
    : QWidget(parent)
    , dptr(new HWebViewPrivate(this))
{
    ZulipWebView *webView = [[ZulipWebView alloc] init];
    [webView setQWidget:this];
    setupLayout((__bridge void *)webView, this);

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

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    dptr->webView = webView;

    dptr->delegate = [[ZulipWebDelegate alloc] initWithPrivate:dptr];
    [dptr->webView setPolicyDelegate:dptr->delegate];
    [dptr->webView setResourceLoadDelegate:dptr->delegate];
    [dptr->webView setFrameLoadDelegate:dptr->delegate];
    [dptr->webView setUIDelegate:dptr->delegate];
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

    dptr->originalURL = url;
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

void HWebView::reload() {
    [[dptr->webView mainFrame] reload];
}

#include "HWebView_mac.moc"
