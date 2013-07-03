#ifndef PLATFORM_INTERFACE_H
#define PLATFORM_INTERFACE_H

#include <QObject>
#include <QString>

/**
  Generic interface to platform-specific bits of the Humbug app
  */

class PlatformInterfacePrivate;

class PlatformInterface : public QObject {
    Q_OBJECT
public:
    PlatformInterface(QObject* parent = 0);
    virtual ~PlatformInterface();

    void desktopNotification(const QString& title, const QString& content);

    void unreadCountUpdated(int oldCount, int newCount);
public slots:
    // Noop on linux
    void checkForUpdates();

private:
    PlatformInterfacePrivate *m_d;
};

#endif
