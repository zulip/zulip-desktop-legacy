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
    bool supportsNotifications();

public slots:
    void userQuit();
    void trayClicked();
    void addJavaScriptObject();
    void linkClicked(QUrl url);

private:
    Ui::HumbugWindow *ui;
    HumbugWebBridge *wb;
    QUrl *start;
};

#endif // HUMBUGWINDOW_H
