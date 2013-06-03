#ifndef HUMBUGWINDOW_H
#define HUMBUGWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "HumbugWebBridge.h"


namespace Ui
{
class HumbugWindow;
}

class HumbugWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit HumbugWindow(QWidget *parent = 0);
    ~HumbugWindow();

public slots:
    void userQuit();
    void trayClicked();
    void addJavaScriptObject();
    void linkClicked(const QUrl &url);
    void updateIcon(int current, int previous);
    void displayPopup(const QString& title, const QString& content);

private:
    Ui::HumbugWindow *m_ui;
    HumbugWebBridge *m_bridge;
    HumbugTrayIcon *m_tray;
    QUrl start;
};

#endif // HUMBUGWINDOW_H
