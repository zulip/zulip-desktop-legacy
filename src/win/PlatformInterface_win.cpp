#include "PlatformInterface.h"

#include "Config.h"
#include "HumbugApplication.h"
#include "HumbugWindow.h"
#include "IconRenderer.h"

#include <shobjidl.h>
#include <windows.h>
#include <mmsystem.h>

// QtSparkle
#include <updater.h>

#include <QIcon>
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QTemporaryFile>
#include <QResource>

class PlatformInterfacePrivate : public QObject {
    Q_OBJECT
public:
    PlatformInterfacePrivate(PlatformInterface *qq) : QObject(qq), q(qq), updater(0) {
        setupTaskbarIcon();
        
        sound_temp.open();
        QResource memory_soundfile(":/audio/humbug.wav");
        sound_temp.write((char*) memory_soundfile.data(), memory_soundfile.size());
        sound_temp.flush();
        sound_temp.close();

        QTimer::singleShot(0, this, SLOT(setupQtSparkle()));
    }
    
    virtual ~PlatformInterfacePrivate() {
    }
    
    void setupQtSparkle() {   
        updater = new qtsparkle::Updater(QUrl("https://humbughq.com/dist/apps/win/sparkle.xml"), APP->mainWindow());
    }

    void setupTaskbarIcon() {
        m_taskbarInterface = NULL;

        // Compute the value for the TaskbarButtonCreated message
        m_IDTaskbarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

        HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, reinterpret_cast<void**> (&(m_taskbarInterface)));

        if (SUCCEEDED(hr)) {
            hr = m_taskbarInterface->HrInit();

            if (FAILED(hr)) {
                m_taskbarInterface->Release();
                m_taskbarInterface = NULL;
            }
        }
    }

    void setOverlayIcon(const QIcon& icon, const QString& description) {
        qDebug() << "Setting windows overlay icon!";
        if (m_taskbarInterface) {
            qDebug() << "Doing it now!";
            HICON overlay_icon = icon.isNull() ? NULL : icon.pixmap(48).toWinHICON();
            m_taskbarInterface->SetOverlayIcon(APP->mainWindow()->winId(), overlay_icon, description.toStdWString().c_str());

            if (overlay_icon) {
                DestroyIcon(overlay_icon);
                return;
            }
        }

        return;
    }

    PlatformInterface *q;
    qtsparkle::Updater *updater;
    QTemporaryFile sound_temp;

    unsigned int m_IDTaskbarButtonCreated;
    ITaskbarList3* m_taskbarInterface;

};

PlatformInterface::PlatformInterface(QObject *parent)
    : QObject(parent)
    , m_d(new PlatformInterfacePrivate(this))
{

}

PlatformInterface::~PlatformInterface() {
    delete m_d;
}

void PlatformInterface::checkForUpdates() {
    if (m_d->updater)
        m_d->updater->CheckNow();
}

void PlatformInterface::desktopNotification(const QString &title, const QString &content) {
    FLASHWINFO finfo;
    finfo.cbSize = sizeof( FLASHWINFO );
    finfo.hwnd = APP->mainWindow()->winId();
    finfo.uCount = 1;         // Flash 40 times
    finfo.dwTimeout = 400; // Duration in milliseconds between flashes
    finfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG; //Flash all, until window comes to the foreground
    ::FlashWindowEx( &finfo );
}

void PlatformInterface::unreadCountUpdated(int oldCount, int newCount) {
    if (newCount == 0)
        m_d->setOverlayIcon(QIcon(), "");
    else
        m_d->setOverlayIcon(APP->mainWindow()->iconRenderer()->winBadgeIcon(newCount), tr("%1 unread messages").arg(newCount));
}

void PlatformInterface::playSound() {
    LPCTSTR filename = reinterpret_cast<const wchar_t *>(m_d->sound_temp.fileName().utf16());
    PlaySound(filename, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
}

#include "PlatformInterface_win.moc"
