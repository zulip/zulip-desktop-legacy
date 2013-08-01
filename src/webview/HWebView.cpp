#include "HWebView.h"

#include "cookiejar.h"
#include "ZulipWebBridge.h"
#include "Config.h"

#include <QDir>
#include <QDesktopServices>
#include <QWebView>
#include <QWebFrame>
#include <QVBoxLayout>

class HWebViewPrivate : public QObject {
    Q_OBJECT
public:
    HWebViewPrivate(HWebView* qq)
        : QObject(qq),
          q(qq),
          webView(new QWebView(qq)),
          bridge(new ZulipWebBridge(qq))
    {
        webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

        QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
        CookieJar *m_cookies = new CookieJar(data_dir.absoluteFilePath("default.dat"));
        webView->page()->networkAccessManager()->setCookieJar(m_cookies);

        connect(webView, SIGNAL(linkClicked(QUrl)), q, SIGNAL(linkClicked(QUrl)));
        connect(bridge, SIGNAL(doDesktopNotification(QString,QString)), q, SIGNAL(desktopNotification(QString,QString)));
        connect(bridge, SIGNAL(doUpdateCount(int)), q, SIGNAL(updateCount(int)));
        connect(bridge, SIGNAL(doUpdatePMCount(int)), q, SIGNAL(updatePMCount(int)));
        connect(bridge, SIGNAL(doBell()), q, SIGNAL(bell()));

        connect(webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(addJavaScriptObject()));
    }
    ~HWebViewPrivate() {}

private slots:
    void addJavaScriptObject() {
        // Ref: http://www.developer.nokia.com/Community/Wiki/Exposing_QObjects_to_Qt_Webkit

        // Don't expose the JS bridge outside our start domain
        if (webView->url().host() != startUrl.host()) {
            return;
        }

        webView->page()->mainFrame()->addToJavaScriptWindowObject("bridge", bridge);

        // QtWebkit has a bug where it won't fall back properly on fonts.
        // If the first font listed in a CSS font-family declaration isn't
        // found, instead of trying the others it simply resorts to a
        // default font immediately.
        //
        // This workaround ensures we still get a fixed-width font for
        // code blocks. jQuery isn't loaded yet when this is run,
        // so we use a DOM method instead of $(...)
        webView->page()->mainFrame()->evaluateJavaScript(
            "document.addEventListener('DOMContentLoaded',"
            "    function () {"
            "        var css = '<style type=\"text/css\">'"
            "                + 'code,pre{font-family:monospace !important;}'"
            "                + '</style>';"
            "        $('head').append(css);"
            "   }"
            ");"
        );
    }

public:
    QWebView* webView;
    HWebView* q;
    ZulipWebBridge* bridge;
    QUrl startUrl;
};

HWebView::HWebView(QWidget *parent)
    : QWidget(parent)
    , dptr(new HWebViewPrivate(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    setLayout(layout);

    layout->addWidget(dptr->webView);

}

HWebView::~HWebView()
{
}

void HWebView::load(const QUrl &url) {
    dptr->startUrl = url;
    dptr->webView->load(url);
}

void HWebView::setUrl(const QUrl &url) {
    load(url);
}

QUrl HWebView::url() const {
    return dptr->webView->url();
}

void HWebView::loadHTML(const QString &html) {
    dptr->webView->setHtml(html);
}

#include "HWebView.moc"
