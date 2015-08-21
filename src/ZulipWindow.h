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
class QToolbarTabDialog;
class GeneralPreferences;
class NotificationPreferences;

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
    void addNewDomainSelection(const QString& url);

public slots:
    void closeEvent(QCloseEvent *);

    void userQuit();
    void showAbout();
    void showPrefs();
    void trayClicked();
    void linkClicked(const QUrl &url);
    void countUpdated(int newCount);
    void pmCountUpdated(int newCount);
    void displayPopup(const QString& title, const QString& content, const QString &source);
    void addNewDomain();

private slots:
    void domainSelected(const QString& domain);
    void reload();

    void animateTray();

    void savePreferences();
    void preferencesLinkClicked(const QString& hashLink);

private:
    void setupTray();
    void setupPrefsWindow();

    void startTrayAnimation(const QList<QIcon>& stages);
    void stopTrayAnimation();

    void readSettings();

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
    QWeakPointer <QToolbarTabDialog> m_preferencesDialog;
    QWeakPointer <GeneralPreferences> m_generalPrefs;
    QWeakPointer <NotificationPreferences> m_notificationPrefs;
    QWeakPointer <QMenu> m_domain_menu;

    CookieJar *m_cookies;
    QUrl m_start;

    int m_unreadCount, m_unreadPMCount;

    PlatformInterface *m_platform;
};

#endif // ZULIPWINDOW_H
