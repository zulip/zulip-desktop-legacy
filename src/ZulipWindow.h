#ifndef ZULIPWINDOW_H
#define ZULIPWINDOW_H

#include "cookiejar.h"
#include "Config.h"

#include <QMainWindow>
#include <QSystemTrayIcon>

class QAction;
namespace Ui
{
class ZulipWindow;
}

class QCloseEvent;
class QSystemTrayIcon;
class HWebView;
class QSignalMapper;
class IconRenderer;
class PlatformInterface;

class ZulipWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ZulipWindow(QWidget *parent = 0);
    void setUrl(const QUrl &url);
    ~ZulipWindow();

    HWebView* webView() const;
    IconRenderer *iconRenderer() const;
    QSystemTrayIcon *trayIcon() const;

public slots:
    void closeEvent(QCloseEvent *);

    void userQuit();
    void showAbout();
    void trayClicked();
    void linkClicked(const QUrl &url);
    void countUpdated(int newCount);
    void pmCountUpdated(int newCount);
    void displayPopup(const QString& title, const QString& content);

private slots:
    void domainSelected(const QString& domain);
    void setStartAtLogin(bool start);
    void showSystemTray(bool show);
    void setBounceDockIcon(bool bounce);

    void animateTray();

private:
    void setupTray();

    void startTrayAnimation(const QList<QIcon>& stages);
    void stopTrayAnimation();

    void readSettings();
    QString domainToUrl(const QString& domain) const;

    Ui::ZulipWindow *m_ui;

    // Tray icon
    IconRenderer *m_renderer;
    QSystemTrayIcon *m_tray;
    QTimer *m_trayTimer;
    int m_animationStep;
    QList<QIcon> m_animationStages;

    // Menu
    QSignalMapper *m_domainMapper;
    QHash<QString, QAction*> m_domains;
    QAction *m_checkForUpdates;
    QAction *m_startAtLogin;
    QAction *m_showSysTray;
    QAction *m_bounceDock;

    CookieJar *m_cookies;
    QUrl m_start;

    int m_unreadCount, m_unreadPMCount;

    PlatformInterface *m_platform;
};

#endif // ZULIPWINDOW_H
