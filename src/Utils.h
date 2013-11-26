#ifndef UTILS_H
#define UTILS_H

#include <qjson/parser.h>

#include <QStringList>
#include <QHash>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QDebug>
#include <QUrl>

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
}

#endif // UTILS_H
