#include "HWebView.h"

#include "cookiejar.h"
#include "ZulipWebBridge.h"
#include "Config.h"
#include "ZulipApplication.h"
#include "Utils.h"
#include "ZulipWindow.h"

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QDir>
#include <QDesktopServices>
#include <QDebug>
#include <QHash>
#include <QHttpMultiPart>
#include <QImage>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QWebView>
#include <QWebPage>
#include <QWebFrame>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <QKeyEvent>

#include <qjson/parser.h>

// QWebkit, WHY DO YOU MAKE ME DO THIS
typedef QHash<QNetworkReply *, QByteArray *> PayloadHash;
typedef QHash<QNetworkReply *, QBuffer *> PayloadBufferHash;
Q_GLOBAL_STATIC(PayloadHash, s_payloads);
Q_GLOBAL_STATIC(PayloadBufferHash, s_payloadBuffers);
Q_GLOBAL_STATIC(PayloadHash, s_imageUploadPayloads);

static QString csrfTokenFromWebPage(QWebPage *page, QUrl baseUrl) {
    QList<QNetworkCookie> cookies = page->networkAccessManager()->cookieJar()->cookiesForUrl(baseUrl);
    foreach (const QNetworkCookie &cookie, cookies) {
        if (cookie.name() == "csrftoken") {
            return cookie.value();
        }
    }

    return QString();
}

// Only install this on a QWebView!
class KeyPressEventFilter : public QObject
{
    Q_OBJECT
public:
    KeyPressEventFilter(QWebView *webview)
        : QObject(webview)
        , m_webView(webview)
    {
    }

    bool eventFilter(QObject *obj, QEvent *ev) {
        if (ev->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);

            if (keyEvent->matches(QKeySequence::Paste)) {
                QClipboard *clipboard = QApplication::clipboard();

                if (clipboard->mimeData()->hasImage()) {
                    const QString csrfToken = csrfTokenFromWebPage(m_webView->page(), m_webView->page()->mainFrame()->url());
                    QNetworkReply *imageUpload = uploadImageFromClipboard(csrfToken);

                    connect(imageUpload, SIGNAL(finished()), this, SLOT(imageUploadFinished()));
                    connect(imageUpload, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(imageUploadProgress(qint64, qint64)));
                    connect(imageUpload, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(imageUploadError(QNetworkReply::NetworkError)));

                    return true;
                }
            }
        }

        return false;
    }

private slots:
    void imageUploadFinished() {
        QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        if (s_imageUploadPayloads()->contains(reply)) {
            delete s_imageUploadPayloads()->take(reply);
        }

        qDebug() << "Image Upload Finished!!";

        QJson::Parser p;
        bool ok;
        QVariant data = p.parse(reply, &ok);

        if (ok) {
            QVariantMap result = data.toMap();
            const QString imageLink = result.value("uri", QString()).toString();

            m_webView->page()->mainFrame()->evaluateJavaScript(QString("compose.uploadFinished(0, undefined, {\"uri\": \"%1\"}, undefined);")
                                                                       .arg(imageLink));
            return;
        }

        m_webView->page()->mainFrame()->evaluateJavaScript("compose.uploadError('Unknown Error');");

    }

    void imageUploadProgress(qint64 bytesSent ,qint64 bytesTotal) {
        // Trigger progress bar change
        int percent = (bytesSent * 100 / bytesTotal);
        m_webView->page()->mainFrame()->evaluateJavaScript(QString("compose.progressUpdated(0, undefined, \"%1\");").arg(percent));

     }

     void imageUploadError(QNetworkReply::NetworkError error) {
         qDebug() << "Got Upload error:" << error;
         m_webView->page()->mainFrame()->evaluateJavaScript("compose.uploadError('Unknown Error');");
     }

private:
    QNetworkReply* uploadImageFromClipboard(const QString &csrfToken) {
        QBuffer *buffer;
        QByteArray *imgData;
        const QString suffix;
        const QString mimetype;

        {
            // Delete QImage after we're done with it
            QClipboard *clipboard = QApplication::clipboard();

            // Write image data to QBuffer
            QImage img = qvariant_cast<QImage>(clipboard->mimeData()->imageData());
            imgData = new QByteArray(img.byteCount(), Qt::Uninitialized);
            buffer = new QBuffer(imgData);
            buffer->open(QIODevice::WriteOnly);
            img.save(buffer, "PNG");
            buffer->close();
            buffer->open(QIODevice::ReadOnly);
        }

        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"csrfmiddlewaretoken\""));
        textPart.setBody(csrfToken.toUtf8());

        QHttpPart imagePart;
        imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
        imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"pasted-image\""));
        // Set PNG image in QBuffer as body part of QHttpPart
        imagePart.setBodyDevice(buffer);
        // The buffer needs to live as long as the HttpPart object, so parent it to the multi part
        buffer->setParent(multiPart);

        multiPart->append(textPart);
        multiPart->append(imagePart);

        // Create POST request to /json/upload_file
        QUrl url(m_webView->page()->mainFrame()->url());
        url.setPath("/json/upload_file");
        url.addQueryItem("mimetype",  "image/png");
        QNetworkRequest request(url);
        request.setRawHeader("Origin", m_webView->page()->mainFrame()->url().toString().toUtf8());
        request.setRawHeader("Referer", m_webView->page()->mainFrame()->url().toString().toUtf8());

        QNetworkReply *reply = m_webView->page()->networkAccessManager()->post(request, multiPart);
        multiPart->setParent(reply); // Delete the QHttpMultiPart with the reply

        s_imageUploadPayloads()->insert(reply, imgData);

        // Trigger start
        m_webView->page()->mainFrame()->evaluateJavaScript("compose.uploadStarted();");
        return reply;
    }

    QWebView *m_webView;
};


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
        // Disable automatic redirection for local server until bugs are ironed out
        if (op == PostOperation && request.url().path().contains("/accounts/logout")) {
            APP->setExplicitDomain(QString());
        }

        return QNetworkAccessManager::createRequest(op, request, outgoingData);

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
        const QString csrfToken = csrfTokenFromWebPage(m_csrfWebView->page(), QUrl(m_siteBaseUrl));

        if (!csrfToken.isEmpty()) {
            m_savedPayload["csrfmiddlewaretoken"] = csrfToken;
            loginToRealHost();
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

class ZulipActionsWebView : public QWebView {
    Q_OBJECT
public:
    explicit ZulipActionsWebView(QWidget* parent = 0)
        : QWebView(parent)
        {
        }

protected:
        void contextMenuEvent(QContextMenuEvent * ev) {
            const  QPoint globalPos = ev->globalPos();

            QMenu *menu = page()->createStandardContextMenu();
            if (menu) {
                // Remove actions that don't do anything
                QStringList blacklisted = QStringList() << "Open Link" <<
                                                           "Open in New Window" <<
                                                           "Save Link...";
                foreach (QAction *action, menu->actions()) {
                    if (blacklisted.contains(action->text())) {
                        menu->removeAction(action);
                    }
                }

                // If the system tray is hidden, show the tray actions
                // in the context menu of the web view
                QSystemTrayIcon *tray = APP->mainWindow()->trayIcon();
                if (tray && !tray->isVisible()) {
                    menu->addSeparator();
                    menu->addActions(tray->contextMenu()->actions());
                }

                menu->exec(globalPos);
                delete menu;
            }
        }
};

class HWebViewPrivate : public QObject {
    Q_OBJECT
public:
    HWebViewPrivate(HWebView* qq)
        : QObject(qq),
          webView(new ZulipActionsWebView(qq)),
          q(qq),
          bridge(new ZulipWebBridge(qq))
    {
        webView->setPage(new LoggingPage(webView));
        webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
        if (APP->debugMode()) {
            webView->page()->setProperty("_q_webInspectorServerPort", 8888);
        }

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

        webView->installEventFilter(new KeyPressEventFilter(webView));
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

void HWebView::reload()
{
    dptr->webView->reload();
}


#include "HWebView.moc"
