#ifndef UTILS_H
#define UTILS_H

#include <qjson/parser.h>

#include <QStringList>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QDebug>
#include <QUrl>

class QNetworkAccessManager;

namespace Utils {

    static inline QHash<QString, QString> parseURLParameters(const QString& url) {
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

    static inline QString parametersDictToString(const QHash<QString, QString>& parameters) {
        QStringList parameterList;
        foreach (const QString key, parameters.keys()) {
            parameterList.append(QString("%1=%2").arg(QString::fromUtf8(QUrl::toPercentEncoding(key)))
                                                 .arg(QString::fromUtf8(QUrl::toPercentEncoding(parameters[key]))));
        }
        return parameterList.join("&");
    }

    // Does a SYNCHRONOUS HTTP request for the base url
    static inline QString baseUrlForEmail(QNetworkAccessManager *nam, const QString& email) {
        QString fetchURL = QString("https://api.zulip.com/v1/deployments/endpoints?email=%1").arg(email);
        QNetworkReply *reply = nam->get(QNetworkRequest(QUrl(fetchURL)));

        // Ugh
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec(QEventLoop::ExcludeUserInputEvents);

        QJson::Parser p;
        QVariantMap result = p.parse(reply).toMap();

        result = result.value("result", QVariantMap()).toMap();
        return result.value("base_site_url", QString()).toString();
    }
}

#endif // UTILS_H
