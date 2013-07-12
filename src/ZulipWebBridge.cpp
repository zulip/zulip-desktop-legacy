#include "ZulipWebBridge.h"
#include <QSystemTrayIcon>
#include <QSysInfo>
#include <stdio.h>
#include "ZulipWindow.h"

ZulipWebBridge::ZulipWebBridge(QObject *parent) :
    QObject(parent)
{
}

QVariantMap ZulipWebBridge::systemInfo()
{
    return QVariantMap();
}

void ZulipWebBridge::desktopNotification(const QString& title, const QString& content)
{
    emit doDesktopNotification(title, content);
}

void ZulipWebBridge::bell()
{
    emit doBell();
}

void ZulipWebBridge::updateCount(int count)
{
    emit doUpdateCount(count);
}

void ZulipWebBridge::updatePMCount(int count)
{
    emit doUpdatePMCount(count);
}
