#include "PlatformInterface.h"
#include "Config.h"
#include "ZulipApplication.h"
#include "ZulipWindow.h"

#include <dbus_notifications.h>

#include <QTemporaryFile>
#include <QResource>
#include <QSystemTrayIcon>

#if defined(QT4_BUILD)
#include <phonon/MediaObject>
#include <phonon/MediaSource>
#include <phonon/AudioOutput>
#elif defined(QT5_BUILD)
#include <QMediaPlayer>
#endif

class PlatformInterfacePrivate : public QObject {
    Q_OBJECT
public:
    PlatformInterfacePrivate(PlatformInterface *qq) : q(qq) {
        sound_temp.open();
        QResource memory_soundfile(":/audio/zulip.ogg");
        sound_temp.write((char*) memory_soundfile.data(), memory_soundfile.size());
        sound_temp.flush();
        sound_temp.close();

#if defined(QT4_BUILD)
        bellsound = new Phonon::MediaObject(q);
        Phonon::createPath(bellsound, new Phonon::AudioOutput(Phonon::MusicCategory, q));

        bellsound->setCurrentSource(Phonon::MediaSource(sound_temp.fileName()));
#elif defined(QT5_BUILD)
        player = new QMediaPlayer;
        player->setMedia(QUrl::fromLocalFile(sound_temp.fileName()));
        player->setVolume(100);
#endif

        m_notifications = new org::freedesktop::Notifications("org.freedesktop.Notifications",
                                                            "/org/freedesktop/Notifications",
                                                            QDBusConnection::sessionBus(),
                                                            this);
        connect(m_notifications, SIGNAL(NotificationClosed(uint,uint)),
                this, SLOT(notificationClosed(uint,uint)));
        connect(m_notifications, SIGNAL(ActionInvoked(uint,QString)),
                this, SLOT(notificationAction(uint,QString)));
        resetLastNotification();
        m_unreadCount = 0;
    }

    ~PlatformInterfacePrivate() {
        delete m_notifications;
#if defined(QT4_BUILD)
        delete bellsound;
#elif defined(QT5_BUILD)
        delete player;
#endif
    }

    /**
     * @brief Updates the notification message with given data and appends unread count if needed.
     * @param title Title of the notification message
     * @param content Content of the notification message
     */
    void updateNotification(const QString &title, const QString &content) {
        m_lastNotificationTitle = title;
        m_lastNotificationContent = content;
        if (m_unreadCount > 1) {
            m_lastNotificationContent.append(QString(" (%1 more unread)").arg(m_unreadCount - 1));
        }
    }

    void setUnreadCount(int unread) {
        m_unreadCount = unread;
    }

    /**
     * @brief Asynchronously calls the notifications service with current message data.
     * It may either create a new notification or replace one if it exists already.
     */
    void showNotification() {
        QStringList actions;
        QVariantMap hints;
        // Default action is usually a click on the notification
        actions << "default" << "default";
        // Assign normal urgency
        hints["urgency"] = 1;
        QDBusPendingReply<uint> reply = m_notifications->Notify(qAppName(), m_lastNotificationId, "zulip",
                                                                m_lastNotificationTitle, m_lastNotificationContent,
                                                                actions, hints, -1);
        // This call is blocking as we require the notification ID value before the next call, which in case
        // of async call might appear earlier than a reply on the previous call arrives
        reply.waitForFinished();
        if (reply.isError()) {
            // Fallback to tray messages if this didn't work out as expected
            APP->mainWindow()->trayIcon()->showMessage(m_lastNotificationTitle, m_lastNotificationContent);
        } else {
            m_lastNotificationId = reply.value();
        }
    }

#if defined(QT4_BUILD)
    void play() {
        bellsound->play();
    }

    Phonon::MediaObject *bellsound;
#elif defined(QT5_BUILD)
    void play() {
        qDebug() << "TRYING TO PLAY!";
        player->play();
        qDebug() << player->error() << player->errorString() << player->state();
    }

    QMediaPlayer *player;
#endif
    QTemporaryFile sound_temp;

    PlatformInterface *q;

private:
    org::freedesktop::Notifications *m_notifications;
    uint m_lastNotificationId;
    QString m_lastNotificationTitle;
    QString m_lastNotificationContent;
    int m_unreadCount;

    void resetLastNotification() {
        m_lastNotificationId = 0;
        m_lastNotificationTitle.clear();
        m_lastNotificationContent.clear();
    }

private slots:
    void notificationClosed(uint id, uint reason) {
        // Make sure to cleanup if notification was closed.
        if (id == m_lastNotificationId) {
            resetLastNotification();
        }
    }

    void notificationAction(uint id, QString key) {
        // Bring up the main window if the default action (usually a click on notification) was emitted.
        if (id == m_lastNotificationId && key == "default") {
            APP->mainWindow()->trayClicked();
        }
    }
};

PlatformInterface::PlatformInterface(QObject *parent)
    : QObject(parent)
    , m_d(new PlatformInterfacePrivate(this))
{
    desktopNotification("test", "test1", "");
    desktopNotification("test", "test2", "");
}

PlatformInterface::~PlatformInterface() {
    delete m_d;
}

QString PlatformInterface::platformWithVersion() {
    return "Linux " + QString::fromUtf8(ZULIP_VERSION_STRING);
}

QString PlatformInterface::userAgentString() {
    return QString("ZulipDesktop/%1 (Linux)").arg(QString::fromUtf8(ZULIP_VERSION_STRING));
}

void PlatformInterface::checkForUpdates() {
    // Noop
}

void PlatformInterface::desktopNotification(const QString &title, const QString &content, const QString&) {
    m_d->updateNotification(title, content);
    m_d->showNotification();
}

void PlatformInterface::setStartAtLogin(bool start) {
    // Noop
}

void PlatformInterface::unreadCountUpdated(int oldCount, int newCount) {
    m_d->setUnreadCount(newCount);
}

void PlatformInterface::playSound() {
    m_d->play();
}

#include "PlatformInterface.moc"
