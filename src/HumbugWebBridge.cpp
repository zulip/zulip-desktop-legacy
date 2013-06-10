#include "HumbugWebBridge.h"
#include <QSystemTrayIcon>
#include <QSysInfo>
#include <stdio.h>
#include "HumbugWindow.h"

HumbugWebBridge::HumbugWebBridge(QObject *parent) :
    QObject(parent), m_unreadCount(0)
{
}

QVariantMap HumbugWebBridge::systemInfo()
{
    return QVariantMap();
}

void HumbugWebBridge::notify(const QVariant &msg)
{
    QMap<QString, QVariant> map = msg.toMap();
    notificationRequested(map.value("title").toString(), map.value("content").toString());
}

void HumbugWebBridge::bell()
{
    bellTriggered();
}

void HumbugWebBridge::updateCount(int count)
{
    // Stash the old value since we want a getCount() call to return current after the signal is emitted
    int old = m_unreadCount;
    m_unreadCount = count;
    countUpdated(count, old);
}

int HumbugWebBridge::getCount()
{
    return m_unreadCount;
}
