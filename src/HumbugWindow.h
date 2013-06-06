#ifndef HUMBUGWINDOW_H
#define HUMBUGWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "HumbugWebBridge.h"
#include <phonon/MediaObject>

namespace Ui
{
class HumbugWindow;
}

class HumbugWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit HumbugWindow(QWidget *parent = 0);
    void setUrl(const QUrl &url);
    ~HumbugWindow();

public slots:
    void userQuit();
    void trayClicked();
    void addJavaScriptObject();
    void linkClicked(const QUrl &url);
    void updateIcon(int current, int previous);
    void displayPopup(const QString& title, const QString& content);
    void playAudibleBell();

private:
    Ui::HumbugWindow *m_ui;
    HumbugWebBridge *m_bridge;
    HumbugTrayIcon *m_tray;
    QUrl m_start;
    Phonon::MediaObject *m_bellsound;
};

#endif // HUMBUGWINDOW_H
