#include "HumbugWebBridge.h"
#include <QSystemTrayIcon>
#include <QSysInfo>
#include <stdio.h>
#include <QDebug>

HumbugWebBridge::HumbugWebBridge(QObject *parent, QWebView *wv, HumbugTrayIcon *ti) :
    QObject(parent)
{
    webView = wv;
    trayIcon = ti;
    unreadCount = 0;
}

QVariantMap HumbugWebBridge::systemInfo() {
    QVariantMap info = QVariantMap();
    info["supportsPopups"] = trayIcon->supportsMessages();
    return info;
}

void HumbugWebBridge::notify(const QVariant &msg)
{
    qDebug() << msg;
    //trayIcon->showMessage(msg->value("title").toString(), msg->value("content").toString(), QSystemTrayIcon::Information);
}

void HumbugWebBridge::updateCount(int count)
{
    unreadCount = count;
    printf("Count updated to %i\n", count);
}

int HumbugWebBridge::getCount()
{
    return unreadCount;
}
