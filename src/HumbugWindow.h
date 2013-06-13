#ifndef HUMBUGWINDOW_H
#define HUMBUGWINDOW_H

#include <QMainWindow>
#include <QTemporaryFile>
#include <QSystemTrayIcon>
#include "cookiejar.h"
#include <phonon/MediaObject>

namespace Ui
{
class HumbugWindow;
}

class QCloseEvent;
class HumbugTrayIcon;

class HumbugWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit HumbugWindow(QWidget *parent = 0);
    void setUrl(const QUrl &url);
    ~HumbugWindow();

public slots:
    void closeEvent(QCloseEvent *);

    void userQuit();
    void showAbout();
    void trayClicked();
    void linkClicked(const QUrl &url);
    void updateIcon(int current, int previous);
    void displayPopup(const QString& title, const QString& content);

private:
    void setupTray();
    void setupSounds();

    void readSettings();

    Ui::HumbugWindow *m_ui;
    HumbugTrayIcon *m_tray;
    QUrl m_start;
    Phonon::MediaObject *m_bellsound;
    CookieJar *m_cookies;
    QTemporaryFile m_sound_temp;

};

#endif // HUMBUGWINDOW_H
