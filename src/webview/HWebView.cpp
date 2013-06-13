#include "HWebView.h"

#include "cookiejar.h"
#include "HumbugWebBridge.h"

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
          bridge(new HumbugWebBridge(qq))
    {
        webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

        QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
        CookieJar *m_cookies = new CookieJar(data_dir.absoluteFilePath("default.dat"));
        webView->page()->networkAccessManager()->setCookieJar(m_cookies);

        connect(webView, SIGNAL(linkClicked(QUrl)), q, SIGNAL(linkClicked(QUrl)));
        connect(bridge, SIGNAL(notificationRequested(QString,QString)), q, SIGNAL(notificationRequested(QString,QString)));
        connect(bridge, SIGNAL(countUpdated(int,int)), q, SIGNAL(countUpdated(int,int)));
        connect(bridge, SIGNAL(bellTriggered()), q,  SIGNAL(bellTriggered()));

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
    }

public:
    QWebView* webView;
    HWebView* q;
    HumbugWebBridge* bridge;
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

#include "HWebView.moc"
