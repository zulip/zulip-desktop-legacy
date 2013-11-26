#include "Utils.h"

#include <QBuffer>
#include <QImage>
#include <QHttpMultiPart>

// Object to stick around with a waiting slot for the network request to finish
// TODO port ugly synchronous event loop code above to use this
namespace Utils {
    class NetworkWatcher : public QObject {
        Q_OBJECT
    public:
        NetworkWatcher(QNetworkReply *reply, QObject *replyObj, QObject *parent = 0)
            : QObject(parent),
              m_reply(reply),
              m_replyObject(replyObj)
        {
            qRegisterMetaType<ConnectionStatus>("ConnectionStatus");
        }

    public slots:
        void execute() {
            if (!m_reply) {
                return;
            }

            int responseCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            ConnectionStatus connection;

            if (m_reply->error() != QNetworkReply::NoError) {
                connection = Offline;
            } else if (responseCode == 204) {
                connection = Online;
            } else {
                connection = Captive;
            }
            qDebug() << "Found network connection status:" << connection << responseCode << m_reply->error() << m_reply->errorString();
            QMetaObject::invokeMethod(m_replyObject, "connectionStatusSlot", Qt::DirectConnection, Q_ARG(Utils::ConnectionStatus, connection));

            if (m_reply->manager()->property("CreatedByUs").toBool()) {
                m_reply->manager()->deleteLater();
            }
            m_reply->deleteLater();
            m_reply = 0;

            deleteLater();
        }

    private:
        QNetworkReply *m_reply;
        QObject *m_replyObject;

    };
}

QHash<QString, QString> Utils::parseURLParameters(const QString& url) {
    QHash<QString, QString> dict;

    foreach (const QString kvs, url.split("&")) {
        if (!kvs.contains("=")) {
            continue;
        }

        QStringList parts = kvs.split("=", QString::KeepEmptyParts);
        if (parts.size() != 2) {
            continue;
        }

        dict.insert(QUrl::fromPercentEncoding(parts[0].toUtf8()), QUrl::fromPercentEncoding(parts[1].toUtf8()));

    }
    return dict;
}

QString Utils::parametersDictToString(const QHash<QString, QString>& parameters) {
    QStringList parameterList;
    foreach (const QString key, parameters.keys()) {
        parameterList.append(QString("%1=%2").arg(QString::fromUtf8(QUrl::toPercentEncoding(key)))
                                             .arg(QString::fromUtf8(QUrl::toPercentEncoding(parameters[key]))));
    }
    return parameterList.join("&");
}

QString Utils::baseUrlForEmail(QNetworkAccessManager *nam, const QString& email, bool *requestSuccessful) {
    QString fetchURL = QString("https://zulip.com/api/v1/deployments/endpoints?email=%1").arg(email);

    bool createdNam = false;
    if (!nam) {
        createdNam = true;
        nam = new QNetworkAccessManager();
    }

    QNetworkReply *reply = nam->get(QNetworkRequest(QUrl(fetchURL)));

    // Ugh
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (createdNam) {
        nam->deleteLater();
    }

    // If we get anything outside a 404 (user deployment not known) or 200,
    // we assume that our attempt at preflight failed because we could not connect
    // to zulip.com
    if (reply->error() != QNetworkReply::NoError &&
        reply->error() != QNetworkReply::ContentNotFoundError &&
        reply->error() != QNetworkReply::UnknownContentError) { /*  UnknownContentError is a server 503 */
        *requestSuccessful = false;
        return QString();
    }

    *requestSuccessful = true;
    QJson::Parser p;
    QVariantMap result = p.parse(reply).toMap();

    result = result.value("result", QVariantMap()).toMap();
    QString domain = result.value("base_site_url", QString()).toString();
    // Only allow http:// on localhost for testing
    if (domain.startsWith("http://") && !domain.startsWith("http://localhost:")) {
        domain.replace("http://", "https://");
    } else if (domain.length() > 0 && !domain.startsWith("https://")) {
        domain = "https://" + domain;
    }

    return domain;
}

void Utils::connectedToInternet(QNetworkAccessManager *nam, QObject *replyObj) {
    if (!nam) {
        nam = new QNetworkAccessManager();
        nam->setProperty("CreatedByUs", true);
    }


    QNetworkReply *reply = nam->get(QNetworkRequest(QUrl("http://www.gstatic.com/generate_204")));
    NetworkWatcher *watcher = new NetworkWatcher(reply, replyObj);

    QObject::connect(reply, SIGNAL(finished()), watcher, SLOT(execute()));
}

Utils::UploadData Utils::uploadImage(const QImage &img, const QString& csrfToken, const QString& originURL, QNetworkAccessManager *nam) {
    // Write image data to QBuffer
    QByteArray *imgData = new QByteArray(img.byteCount(), Qt::Uninitialized);
    QBuffer *buffer = new QBuffer(imgData);
    buffer->open(QIODevice::WriteOnly);
    img.save(buffer, "PNG");
    buffer->close();
    buffer->open(QIODevice::ReadOnly);

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
    QUrl url(originURL);
    url.setPath("/json/upload_file");
    url.addQueryItem("mimetype",  "image/png");
    QNetworkRequest request(url);
    request.setRawHeader("Origin", originURL.toUtf8());
    request.setRawHeader("Referer", originURL.toUtf8());

    if (!nam) {
        // Delete our created NAM when the reply is deleted
        nam = new QNetworkAccessManager(multiPart);
    }

    QNetworkReply *reply = nam->post(request, multiPart);
    multiPart->setParent(reply); // Delete the QHttpMultiPart with the reply

    Utils::UploadData data(reply, imgData);
    return data;
}

#include "Utils.moc"
