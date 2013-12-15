#ifndef UTILS_H
#define UTILS_H

#include <QStringList>
#include <QHash>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QImage>
#include <QNetworkReply>
#include <QEventLoop>
#include <QPair>
#include <QDebug>
#include <QUrl>

class QBuffer;

namespace Utils {

    QHash<QString, QString> parseURLParameters(const QString& url);

    QString parametersDictToString(const QHash<QString, QString>& parameters);

    // Does a SYNCHRONOUS HTTP request for the base url
    QString baseUrlForEmail(QNetworkAccessManager *nam, const QString& email, bool *requestSuccessful);

    enum ConnectionStatus {
        Offline = 0, // No internet connection
        Captive,     // Online but captive, LAN redirecting request
        Online       // Connected to external internet
    };

    void connectedToInternet(QNetworkAccessManager *nam, QObject *replyObj);

    typedef QPair<QNetworkReply *, QByteArray *> UploadData;
    UploadData uploadImage(const QImage &img, const QString &csrfToken, const QString &originURL, QNetworkAccessManager *nam);

    QNetworkReply *uploadImageFromBuffer(QBuffer *buffer, const QString &csrfToken, const QString &originURL, QNetworkAccessManager *nam);
}

#endif // UTILS_H
