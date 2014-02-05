#include "PlatformInterface.h"

#include "Config.h"
#include "ZulipApplication.h"
#include "ZulipWindow.h"
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
#include <QSystemTrayIcon>

class PlatformInterfacePrivate : public QObject {
    Q_OBJECT
public:
    PlatformInterfacePrivate(PlatformInterface *qq) : QObject(qq), q(qq), updater(0) {
        setupTaskbarIcon();

        sound_temp.open();
        QResource memory_soundfile(":/audio/zulip.wav");
        sound_temp.write((char*) memory_soundfile.data(), memory_soundfile.size());
        sound_temp.flush();
        sound_temp.close();

        QTimer::singleShot(0, this, SLOT(setupQtSparkle()));
    }

    virtual ~PlatformInterfacePrivate() {
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
        if (m_taskbarInterface) {
            HICON overlay_icon = icon.isNull() ? NULL : icon.pixmap(48).toWinHICON();
            m_taskbarInterface->SetOverlayIcon(APP->mainWindow()->winId(), overlay_icon, description.toStdWString().c_str());

            if (overlay_icon) {
                DestroyIcon(overlay_icon);
                return;
            }
        }

        return;
    }

private slots:
    void setupQtSparkle() {
#ifdef SSO_BUILD
        updater = new qtsparkle::Updater(QUrl("https://zulip.com/dist/apps/sso/win/sparkle.xml"), APP->mainWindow());
#else
        updater = new qtsparkle::Updater(QUrl("https://zulip.com/dist/apps/win/sparkle.xml"), APP->mainWindow());
#endif
    }

public:
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

void PlatformInterface::desktopNotification(const QString &title, const QString &content, const QString& source) {
    // On Windows/Linux, let Qt show the (default, crappy) notification message
    // until we have a better one
    APP->mainWindow()->trayIcon()->showMessage(title, content);

    if (shouldBounceFromSource(source)) {
        FLASHWINFO finfo;
        finfo.cbSize = sizeof( FLASHWINFO );
        finfo.hwnd = APP->mainWindow()->winId();
        finfo.uCount = 1;         // Flash 40 times
        finfo.dwTimeout = 400; // Duration in milliseconds between flashes
        finfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG; //Flash all, until window comes to the foreground
        ::FlashWindowEx( &finfo );
    }
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

// Extract QString error message from a Win32 API error code
void handleSystemError(LONG errorCode) {

    if (errorCode == ERROR_SUCCESS) {
        return;
    }

    LPTSTR errorText = NULL;

    QString errorString;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorText, 0, NULL);

    if (errorText != NULL) {
        errorString = QString::fromWCharArray(errorText);
        LocalFree(errorText);
        errorText = NULL;
    }

    qDebug() << "WIN32 API Error:" << errorCode << errorString;
}

void PlatformInterface::setStartAtLogin(bool start) {
    // We enable start-on-login by writing to the registry,
    // specifically HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run\Zulip
    //
    // In theory QSettings should be able to read/write from the registry, but I was unable
    // to get it to work. So we use the native win32 API calls instead :-/
    HKEY runFolder = NULL;
    LONG ret = RegCreateKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
                              NULL, 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, NULL, &runFolder, NULL);
    handleSystemError(ret);

    if (start) {
        const QString path = QCoreApplication::applicationFilePath().replace('/', '\\');
        wchar_t winPath[path.size()];
        path.toWCharArray(winPath);

       ret = RegSetValueEx(runFolder, L"Zulip", 0, REG_SZ , (BYTE*)winPath, sizeof(winPath));
       handleSystemError(ret);
    } else {
        ret = RegDeleteValue(runFolder, L"Zulip");
        handleSystemError(ret);
    }
    RegCloseKey(runFolder);
}


QString PlatformInterface::platformWithVersion() {
    return "Windows " + QString::fromUtf8(ZULIP_VERSION_STRING);
}

QString PlatformInterface::userAgentString() {
    return QString("ZulipDesktop/%1 (Windows)").arg(QString::fromUtf8(ZULIP_VERSION_STRING));
}


#include "PlatformInterface_win.moc"
