#include "PlatformInterface.h"
#include "Config.h"
#include "ZulipApplication.h"
#include "ZulipWindow.h"

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

class PlatformInterfacePrivate {
public:
    PlatformInterfacePrivate(PlatformInterface * qq) : q(qq) {
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
};

PlatformInterface::PlatformInterface(QObject *parent)
    : QObject(parent)
    , m_d(new PlatformInterfacePrivate(this))
{
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

void PlatformInterface::desktopNotification(const QString &title, const QString &content) {
    APP->mainWindow()->trayIcon()->showMessage(title, content);
}

void PlatformInterface::setStartAtLogin(bool start) {
    // Noop
}

void PlatformInterface::unreadCountUpdated(int, int) {

}

void PlatformInterface::playSound() {
    m_d->play();
}
