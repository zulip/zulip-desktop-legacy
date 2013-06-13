#include "HWebView.h"

#include "Utils.h"

#include <QMacCocoaViewContainer>
#include <QVBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QFile>

#import "Foundation/NSAutoreleasePool.h"
#import "Foundation/NSNotification.h"
#import "Foundation/Foundation.h"
#import "WebKit/WebKit.h"
#import "AppKit/NSSearchField.h"

#define KEYCODE_A 0
#define KEYCODE_X 7
#define KEYCODE_C 8
#define KEYCODE_V 9

@interface HumbugWebDelegate : NSObject
{
    HWebViewPrivate* q;
}
- (id)initWithPrivate:(HWebViewPrivate*)q;
- (void)webView:(WebView *)webView decidePolicyForNewWindowAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request newFrameName:(NSString *)frameName decisionListener:(id < WebPolicyDecisionListener >)listener;
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
            // TODO
            return YES;
        }
        else if (keyCode == KEYCODE_C)
        {
            QClipboard* clipboard = QApplication::clipboard();
            // TODO
            return YES;
        }
        else if (keyCode == KEYCODE_V)
        {
            QClipboard* clipboard = QApplication::clipboard();
            // Simulate a cmd-v
            // TODO
            return YES;
        }
        else if (keyCode == KEYCODE_X)
        {
            QClipboard* clipboard = QApplication::clipboard();
            // TODO
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
    NSLog(@"Getting a decidePolicyForNewWindowAction! %@", [request URL]);
    q->linkClicked(toQUrl([request URL]));

    [listener ignore];
}
@end

HWebView::HWebView(QWidget *parent)
    : QWidget(parent)
    , dptr(new HWebViewPrivate(this))
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    HumbugWebView *webView = [[HumbugWebView alloc] init];
    setupLayout(webView, this);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    dptr->webView = webView;

    dptr->delegate = [[HumbugWebDelegate alloc] initWithPrivate:dptr];
    [dptr->webView setPolicyDelegate:dptr->delegate];

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
