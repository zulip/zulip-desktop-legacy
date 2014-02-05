#include "ZulipWebBridge.h"
#include "PlatformInterface.h"
#include "ZulipWindow.h"

#include <QDebug>
#include <QSystemTrayIcon>
#include <QSysInfo>

ZulipWebBridge::ZulipWebBridge(QObject *parent) :
    QObject(parent)
{
}

QVariantMap ZulipWebBridge::systemInfo()
{
    return QVariantMap();
}

void ZulipWebBridge::desktopNotification(const QString& title, const QString& content, const QString &source)
{
    emit doDesktopNotification(title, content, source);
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

QString ZulipWebBridge::desktopAppVersion()
{
  return PlatformInterface::platformWithVersion();
}

void ZulipWebBridge::log(const QString &msg) {
    qDebug() << "QtWebkit Log:" << msg;
}
