#include "PlatformInterface.h"


PlatformInterface::PlatformInterface(QObject *parent)
    : QObject(parent)
    , m_d(0)
{
}

PlatformInterface::~PlatformInterface() {}

void PlatformInterface::checkForUpdates() {
    // Noop
}

void PlatformInterface::desktopNotification(const QString &, const QString &) {
    // Noop
}

void PlatformInterface::unreadCountUpdated(int, int) {
  
}
