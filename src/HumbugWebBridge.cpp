#include "HumbugWebBridge.h"
#include <QSystemTrayIcon>
#include <QSysInfo>
#include <stdio.h>
#include "HumbugWindow.h"

HumbugWebBridge::HumbugWebBridge(QObject *parent) :
    QObject(parent)
{
}

QVariantMap HumbugWebBridge::systemInfo()
{
    return QVariantMap();
}

void HumbugWebBridge::desktopNotification(const QString& title, const QString& content)
{
    emit doDesktopNotification(title, content);
}

void HumbugWebBridge::bell()
{
    emit doBell();
}

void HumbugWebBridge::updateCount(int count)
{
    emit doUpdateCount(count);
}
