#ifndef PLATFORM_INTERFACE_H
#define PLATFORM_INTERFACE_H

#include "preferences/NotificationPreferences.h"

#include <QObject>
#include <QSettings>
#include <QString>

/**
  Generic interface to platform-specific bits of the Zulip app
  */

class PlatformInterfacePrivate;

class PlatformInterface : public QObject {
    Q_OBJECT
public:
    PlatformInterface(QObject* parent = 0);
    virtual ~PlatformInterface();

    void desktopNotification(const QString& title, const QString& content, const QString& source);
    void unreadCountUpdated(int oldCount, int newCount);

    void setStartAtLogin(bool start);

    static QString userAgentString();
    static QString platformWithVersion();
public slots:
    // Noop on linux
    void checkForUpdates();
    void playSound();

protected:
    bool shouldBounceFromSource(const QString& source) {
        QSettings s;
        const NotificationPreferences::BounceSetting bounceSetting =
                (NotificationPreferences::BounceSetting)s.value("BounceDockIcon", NotificationPreferences::BounceOnPM).toInt();
        // If bounce setting is never, do not bounce AND
        //  If message type is PM or @-mention or alert word, bounce OR
        //  If message type is stream, bounce only if setting is "all"
        const bool specificAlert = source == "pm" || source == "alert" || source == "mention";

        if ((bounceSetting != NotificationPreferences::BounceNever) &&
            (specificAlert ||
             (source == "stream" && bounceSetting == NotificationPreferences::BounceOnAll))) {
            return true;
        }
        return false;
    }

private:
    PlatformInterfacePrivate *m_d;
};

#endif
