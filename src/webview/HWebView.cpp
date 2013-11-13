#include "HWebView.h"

#include "cookiejar.h"
#include "ZulipWebBridge.h"
#include "Config.h"
#include "ZulipApplication.h"
#include "Utils.h"

#include <QDir>
#include <QDesktopServices>
#include <QDebug>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QWebView>
#include <QWebFrame>
#include <QVBoxLayout>
#include <QBuffer>

// QWebkit, WHY DO YOU MAKE ME DO THIS
typedef QHash<QNetworkReply *, QByteArray *> PayloadHash;
typedef QHash<QNetworkReply *, QBuffer *> PayloadBufferHash;
Q_GLOBAL_STATIC(PayloadHash, s_payloads);
Q_GLOBAL_STATIC(PayloadBufferHash, s_payloadBuffers);

class LoggingPage : public QWebPage
{
    Q_OBJECT
public:
    LoggingPage(QObject *parent)
        : QWebPage(parent)
    {
    }

protected:
    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID ) {
        qDebug() << "WebKit Console Message" << message << "Line" << lineNumber << "sourceID" << sourceID;
    }
};

class ZulipNAM : public QNetworkAccessManager
{
    Q_OBJECT
public:
    ZulipNAM(QWebView *webView, QObject *parent = 0)
        : QNetworkAccessManager(parent)
        , m_zulipWebView(webView)
        , m_csrfWebView(0)
        , m_redirectedRequest(false)
    {
        connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(cleanupFinishedReplies(QNetworkReply*)));
    }

protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
    {
#ifndef SSO_BUILD
        // Disable automatic redirection for local server non-SSO builds until bugs are ironed out
        if (op == PostOperation && request.url().path().contains("/accounts/logout")) {
            APP->setExplicitDomain(QString());
        }

        return QNetworkAccessManager::createRequest(op, request, outgoingData);
#endif

        // If this is an original login to the app, we preflight the user
        // to redirect to the site-local Zulip instance if appropriate
        if (!m_redirectedRequest && !APP->explicitDomain() && op == PostOperation &&
            outgoingData && request.url().path() == "/accounts/login/") {
            m_redirectedRequest = false;

            const QByteArray dataBuffer = outgoingData->readAll();
            const QString postBody = QString::fromUtf8(dataBuffer);
            m_savedPayload = Utils::parseURLParameters(postBody);
            QString email = m_savedPayload.value("username", QString());

            if (email.size() == 0) {
                QByteArray *tempData = new QByteArray(dataBuffer);
                QBuffer *tempBuffer = new QBuffer(tempData, 0);
                QNetworkReply *rebuildReply = QNetworkAccessManager::createRequest(op, request, tempBuffer);
                s_payloads()->insert(rebuildReply, tempData);
                s_payloadBuffers()->insert(rebuildReply, tempBuffer);
                return rebuildReply;
            }

            bool requestSuccessful;
            m_siteBaseUrl = Utils::baseUrlForEmail(this, email, &requestSuccessful);

            m_savedOriginalRequest = request;

            if (!requestSuccessful) {
                // Failed to load, so we ask the user directly
                APP->askForCustomServer([=](QString domain) {
                    m_siteBaseUrl = domain;
                    snatchCSRFAndRedirect();
                }, [=] {
                    // Retry
                    m_zulipWebView->load(request, op, dataBuffer);
                });
            } else if (m_siteBaseUrl.length() == 0) {
                QByteArray *tempData = new QByteArray(dataBuffer);
                QBuffer *tempBuffer = new QBuffer(tempData, 0);
                QNetworkReply *rebuildReply = QNetworkAccessManager::createRequest(op, request, tempBuffer);
                s_payloads()->insert(rebuildReply, tempData);
                s_payloadBuffers()->insert(rebuildReply, tempBuffer);
                return rebuildReply;
            }

            snatchCSRFAndRedirect();
            // Create new dummy request
            return QNetworkAccessManager::createRequest(op, QNetworkRequest(QUrl("")), 0);
        }

        if (op == PostOperation && request.url().path().contains("/accounts/logout")) {
            APP->setExplicitDomain(QString());
        }

        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }

private slots:
    void csrfLoadFinished() {
        QList<QNetworkCookie> cookies = m_csrfWebView->page()->networkAccessManager()->cookieJar()->cookiesForUrl(QUrl(m_siteBaseUrl));
        foreach (const QNetworkCookie &cookie, cookies) {
            if (cookie.name() == "csrftoken") {
                m_savedPayload["csrfmiddlewaretoken"] = cookie.value();
                loginToRealHost();
                break;
            }
        }

        m_siteBaseUrl.clear();
        m_csrfWebView->deleteLater();
        m_csrfWebView = 0;
    }
    
    void cleanupFinishedReplies(QNetworkReply* reply) {
        if (s_payloads()->contains(reply)) {
            delete s_payloads()->take(reply);
        }
        if (s_payloadBuffers()->contains(reply)) {
            s_payloadBuffers()->take(reply)->deleteLater();
        }
    }

private:
    QString snatchCSRFToken() {
        // Load the login page for the target Zulip server and extract the CSRF token
        m_csrfWebView = new QWebView();
        // Make sure to share the same NAM---then we use the same Cookie Jar and when we
        // make our redirected POST request, our CSRF token in our cookie matches with
        // the value in the POST
        m_csrfWebView->page()->setNetworkAccessManager(this);
        QUrl loginUrl(m_siteBaseUrl + "login/");

        QObject::connect(m_csrfWebView, SIGNAL(loadFinished(bool)), this, SLOT(csrfLoadFinished()));
        m_csrfWebView->load(loginUrl);
        return QString();
    }

    void loginToRealHost() {
        QString rebuiltPayload = Utils::parametersDictToString(m_savedPayload);
        m_redirectedRequest = true;
        m_zulipWebView->load(m_savedRequest, QNetworkAccessManager::PostOperation, rebuiltPayload.toUtf8());
    }

    void snatchCSRFAndRedirect() {
        QUrl baseSite(m_siteBaseUrl);

        if (baseSite.host() == m_savedOriginalRequest.url().host()) {
            return;
        }

        qDebug() << "Got different base URL, redirecting" << baseSite;
        APP->setExplicitDomain(m_siteBaseUrl);

        QUrl newUrl(m_savedOriginalRequest.url());
        newUrl.setHost(baseSite.host());
        m_savedRequest = QNetworkRequest(m_savedOriginalRequest);
        m_savedRequest.setUrl(newUrl);
        m_savedRequest.setRawHeader("Origin", newUrl.toEncoded());
        m_savedRequest.setRawHeader("Referer", newUrl.toEncoded());

        // Steal CSRF token
        // This will asynchronously load the webpage in the background
        // and then redirect the user
        snatchCSRFToken();
    }

    QString m_siteBaseUrl;
    QWebView *m_zulipWebView;
    QWebView *m_csrfWebView;

    QNetworkRequest m_savedOriginalRequest;
    QNetworkRequest m_savedRequest;
    QHash<QString, QString> m_savedPayload;

    bool m_redirectedRequest;
};

class HWebViewPrivate : public QObject {
    Q_OBJECT
public:
    HWebViewPrivate(HWebView* qq)
        : QObject(qq),
          webView(new QWebView(qq)),
          q(qq),
          bridge(new ZulipWebBridge(qq))
    {
        webView->setPage(new LoggingPage(webView));
        webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

        QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
        CookieJar *m_cookies = new CookieJar(data_dir.absoluteFilePath("default.dat"));
        ZulipNAM* nam = new ZulipNAM(webView, this);
        webView->page()->setNetworkAccessManager(nam);
        webView->page()->networkAccessManager()->setCookieJar(m_cookies);

        connect(webView, SIGNAL(linkClicked(QUrl)), q, SIGNAL(linkClicked(QUrl)));
        connect(webView, SIGNAL(loadFinished(bool)), this, SLOT(zulipLoadFinished(bool)));
        connect(bridge, SIGNAL(doDesktopNotification(QString,QString)), q, SIGNAL(desktopNotification(QString,QString)));
        connect(bridge, SIGNAL(doUpdateCount(int)), q, SIGNAL(updateCount(int)));
        connect(bridge, SIGNAL(doUpdatePMCount(int)), q, SIGNAL(updatePMCount(int)));
        connect(bridge, SIGNAL(doBell()), q, SIGNAL(bell()));

        connect(webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(addJavaScriptObject()));
        connect(webView->page()->mainFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));
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

    void zulipLoadFinished(bool successful) {
        // Disable local-server firewall guess
        return;

        if (successful) {
            return;
        }

        Utils::connectedToInternet(webView->page()->networkAccessManager(), [=](Utils::ConnectionStatus status) {
            if (status != Utils::Online)
                askForInitialLoadDomain();
        });
    }

    void askForInitialLoadDomain() {
        if (!APP->explicitDomain()) {
            qDebug() << "Failed to load initial Zulip login page, asking directly";
            APP->askForCustomServer([=](QString domain) {
                qDebug() << "Got manually entered domain" << domain << ", redirecting";
                webView->load(QUrl(domain));
            }, [=] () {
                // Retry
                webView->load(startUrl);
            });
        }
    }

    void urlChanged(const QUrl& url) {
        const QString dispatch_path = "accounts/deployment_dispatch";
        if (!lastUrl.path().contains(dispatch_path) ||
            url.path().contains(dispatch_path))
        {
            lastUrl = url;
            return;
        }

        qDebug() << "Got redirect from deployment_dispatch login, saving as explicit domain:" << url;
        APP->setExplicitDomain(url.toString());
        lastUrl = url;
    }


public:
    QWebView* webView;
    HWebView* q;
    ZulipWebBridge* bridge;
    QUrl startUrl;
    QUrl lastUrl;
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
    dptr->lastUrl = url;
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
