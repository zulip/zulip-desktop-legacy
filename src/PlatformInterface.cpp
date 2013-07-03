#include "PlatformInterface.h"

#include <QTemporaryFile>
#include <QResource>

#include <phonon/MediaObject>
#include <phonon/MediaSource>
#include <phonon/AudioOutput>

class PlatformInterfacePrivate {
public:
    PlatformInterfacePrivate(PlatformInterface * qq) : q(qq) {

        bellsound = new Phonon::MediaObject(q);
        Phonon::createPath(bellsound, new Phonon::AudioOutput(Phonon::MusicCategory, q));

        sound_temp.open();
        QResource memory_soundfile(":/humbug.ogg");
        sound_temp.write((char*) memory_soundfile.data(), memory_soundfile.size());
        sound_temp.flush();
        sound_temp.close();

        bellsound->setCurrentSource(Phonon::MediaSource(sound_temp.fileName()));

    }

    Phonon::MediaObject *bellsound;
    QTemporaryFile sound_temp;

    PlatformInterface *q;
};

PlatformInterface::PlatformInterface(QObject *parent)
    : QObject(parent)
    , m_d(new PlatformInterfacePrivate(this))
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

void PlatformInterface::playSound() {
    m_d->bellsound->play();
}
