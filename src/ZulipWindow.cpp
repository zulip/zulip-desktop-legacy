
#include "ZulipWindow.h"

#include "ZulipApplication.h"
#include "ZulipAboutDialog.h"
#include "IconRenderer.h"
#include "ui_ZulipWindow.h"
#include "Config.h"
#include "PlatformInterface.h"
#include "preferences/GeneralPreferences.h"
#include "preferences/NotificationPreferences.h"
#include "thirdparty/qocoa/qtoolbartabdialog.h"

#include <QDir>
#include <QMenuBar>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <QSystemTrayIcon>
#include <QSignalMapper>
#include <QTimer>
#include <QFontDatabase>
#include <QDebug>

static QString s_defaultZulipURL = "https://zulip.com/desktop_home";
static QString s_defaultZulipSSOURL = "https://zulip.com/accounts/deployment_dispatch";

ZulipWindow::ZulipWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::ZulipWindow),
    m_renderer(new IconRenderer(":images/zulip.svg", this)),
    m_trayTimer(new QTimer(this)),
    m_animationStep(0),
    m_domainMapper(new QSignalMapper(this)),
    m_unreadCount(0),
    m_unreadPMCount(0),
    m_platform(new PlatformInterface(this))
{
    m_ui->setupUi(this);

#if defined(QT4_BUILD)
    QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#elif defined(QT5_BUILD)
    QDir data_dir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first();
#endif

    // Create the directory if it doesn't already exist
    data_dir.mkdir(data_dir.absolutePath());

    QFontDatabase::addApplicationFont(":/zulip.ttc");

    statusBar()->hide();

    setupTray();
    setupPrefsWindow();

    connect(m_ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(m_ui->webView, SIGNAL(desktopNotification(QString,QString, QString)), this, SLOT(displayPopup(QString,QString, QString)));
    connect(m_ui->webView, SIGNAL(updateCount(int)), this, SLOT(countUpdated(int)));
    connect(m_ui->webView, SIGNAL(updatePMCount(int)), this, SLOT(pmCountUpdated(int)));
    connect(m_ui->webView, SIGNAL(bell()), m_platform, SLOT(playSound()));

    readSettings();
}


ZulipWindow::~ZulipWindow()
{
    delete m_ui;
}

HWebView* ZulipWindow::webView() const {
    return m_ui->webView;
}

IconRenderer* ZulipWindow::iconRenderer() const {
    return m_renderer;
}

QSystemTrayIcon *ZulipWindow::trayIcon() const {
    return m_tray;
}

void ZulipWindow::setupTray() {
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(m_renderer->icon());

    connect(m_tray, SIGNAL(messageClicked()), this, SLOT(trayClicked()));

    // Animation for tray icon is 2seconds per image
    m_trayTimer->setInterval(2000);
    m_trayTimer->setSingleShot(false);
    connect(m_trayTimer, SIGNAL(timeout()), this, SLOT(animateTray()));

    QMenu *menu = new QMenu(this);
    QAction *about_action = menu->addAction("About");
    about_action->setMenuRole(QAction::AboutRole);
    connect(about_action, SIGNAL(triggered()), this, SLOT(showAbout()));

    QAction *prefs_action = new QAction("Preferences", 0);
    prefs_action->setMenuRole(QAction::PreferencesRole);
    connect(prefs_action, SIGNAL(triggered()), this, SLOT(showPrefs()));

#ifndef Q_OS_MAC
    menu->addAction(prefs_action);
#endif

    if (APP->debugMode()) {
        QMenu* domain_menu = new QMenu("Domain", menu);

        QAction* prod = domain_menu->addAction("Production");
        prod->setCheckable(true);
        connect(prod, SIGNAL(triggered()), m_domainMapper, SLOT(map()));
        m_domains["https://zulip.com"] = prod;

        QAction* staging = domain_menu->addAction("Staging");
        staging->setCheckable(true);
        connect(staging, SIGNAL(triggered()), m_domainMapper, SLOT(map()));
        m_domains["https://staging.zulip.com"] = staging;

        QAction* dev = domain_menu->addAction("Local");
        dev->setCheckable(true);
        connect(dev, SIGNAL(triggered()), m_domainMapper, SLOT(map()));
        m_domains["http://localhost:9991"] = dev;

        m_domainMapper->setMapping(prod, "https://zulip.com");
        m_domainMapper->setMapping(staging, "https://staging.zulip.com");
        m_domainMapper->setMapping(dev, "http://localhost:9991");

        connect(m_domainMapper, SIGNAL(mapped(QString)), this, SLOT(domainSelected(QString)));

        menu->addMenu(domain_menu);
    }

#ifdef Q_OS_WIN
    QAction* checkForUpdates = menu->addAction("Check for Updates...");
    connect(checkForUpdates, SIGNAL(triggered()), m_platform, SLOT(checkForUpdates()));
#endif

    QAction *exit_action = menu->addAction("Exit");
    connect(exit_action, SIGNAL(triggered()), this, SLOT(userQuit()));
    connect(m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClicked()));
    m_tray->setContextMenu(menu);
    m_tray->show();

#ifdef Q_OS_MAC
    // Similar "check for updates" action, but in a different menu
    QMenu* about_menu = menuBar()->addMenu("Zulip");
    about_menu->addAction(about_action);
    about_menu->addAction(prefs_action);

    QAction* checkForUpdates = about_menu->addAction("Check for Updates...");
    checkForUpdates->setMenuRole(QAction::ApplicationSpecificRole);
    connect(checkForUpdates, SIGNAL(triggered()), m_platform, SLOT(checkForUpdates()));
#endif

    QAction *reload = new QAction("Reload", this);
    connect(reload, SIGNAL(triggered(bool)), this, SLOT(reload()));

    menu->insertAction(exit_action, reload);
}

void ZulipWindow::setupPrefsWindow() {
    m_preferencesDialog = QWeakPointer<QToolbarTabDialog>(new QToolbarTabDialog());

    m_generalPrefs = QWeakPointer<GeneralPreferences>(new GeneralPreferences());
    m_preferencesDialog.data()->addTab(m_generalPrefs.data(), QPixmap(":/images/prefs_general_64x64.png"), "General", "General Preferences");

    m_notificationPrefs = QWeakPointer<NotificationPreferences>(new NotificationPreferences());
    connect(m_notificationPrefs.data(), SIGNAL(linkClicked(QString)), this, SLOT(preferencesLinkClicked(QString)));

    m_preferencesDialog.data()->addTab(m_notificationPrefs.data(), QPixmap(":/images/prefs_notifications_64x64.png"), "Notifications", "Notification Preferences");

    m_preferencesDialog.data()->setCurrentIndex(0);
    connect(m_preferencesDialog.data(), SIGNAL(accepted()), this, SLOT(savePreferences()));
}

void ZulipWindow::startTrayAnimation(const QList<QIcon> &stages) {
    m_animationStages = stages;
    m_animationStep = 0;

    m_tray->setIcon(m_animationStages[m_animationStep]);

    m_trayTimer->start();
}

void ZulipWindow::stopTrayAnimation() {
    m_animationStages.clear();
    m_animationStep = 0;

    m_trayTimer->stop();

    m_tray->setIcon(m_renderer->icon(m_unreadCount));
}

void ZulipWindow::animateTray() {
    m_animationStep = (m_animationStep + 1) % m_animationStages.size();
    m_tray->setIcon(m_animationStages[m_animationStep]);
}

void ZulipWindow::closeEvent(QCloseEvent *ev) {
    QSettings settings;
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());
    QMainWindow::closeEvent(ev);
}

void ZulipWindow::readSettings() {
    QSettings settings;
    const QByteArray geometry = settings.value("MainWindow/geometry").toByteArray();
    restoreGeometry(geometry);
    restoreState(settings.value("MainWindow/windowState").toByteArray());

    // If there's no saved geometry, give us enough width to show both sidebars
    if (geometry.isEmpty()) {
        resize(1200, rect().height());
    }

    QString domain = settings.value("Domain").toString();
    QString site = domainToUrl(domain);
    if (site.isEmpty()) {
        domain = "prod";
#ifdef SSO_BUILD
        site = s_defaultZulipSSOURL;
#else
        site = s_defaultZulipURL;
#endif
    } else {
        APP->setExplicitDomain(site);
    }

    m_start = site;
    m_ui->webView->load(m_start);

    if (m_domains.contains(domain))
        m_domains[domain]->setChecked(true);

    bool startAtLoginSetting = settings.value("LaunchAtLogin", true).toBool();
    m_platform->setStartAtLogin(startAtLoginSetting);

    bool showSysTray = settings.value("ShowSystemTray", true).toBool();
    m_tray->setVisible(showSysTray);
    APP->setQuitOnLastWindowClosed(!showSysTray);
}

void ZulipWindow::preferencesLinkClicked(const QString &hashLink) {
    m_preferencesDialog.data()->hide();

    const QUrl currentUrl = webView()->url();

    if (currentUrl.isEmpty()) {
        qDebug() << "Unable to get valid current URL for pref hotlink";
        return;
    }

    // Build a new URL without any path or hash, to go directly to desired place
    const QString urlString = currentUrl.scheme() + "://" + currentUrl.host() + "/" + hashLink;
    const QUrl newUrl(urlString);

    webView()->load(newUrl);
}

void ZulipWindow::savePreferences() {
    QSettings s;

    const bool showTrayIcon = m_generalPrefs.data()->showTrayIcon();
    const bool startAtLogin = m_generalPrefs.data()->startAtLogin();

    m_tray->setVisible(showTrayIcon);

    // Quit when the last window is closed only if there is a tray icon,
    // otherwise there is no way to get it back!
    APP->setQuitOnLastWindowClosed(!showTrayIcon);

    m_platform->setStartAtLogin(startAtLogin);

    s.setValue("ShowSystemTray", showTrayIcon);
    s.setValue("LaunchAtLogin", startAtLogin);

    const NotificationPreferences::BounceSetting bounceSetting = m_notificationPrefs.data()->bounceSetting();
    s.setValue("BounceDockIcon", bounceSetting);
}

void ZulipWindow::setUrl(const QUrl &url)
{
    m_start = url;

    m_ui->webView->load(url);
}

void ZulipWindow::showAbout()
{
    ZulipAboutDialog *d = new ZulipAboutDialog(this);
    d->show();
}

void ZulipWindow::showPrefs()
{
    if (m_preferencesDialog.isNull()) {
        // Still loading?
        return;
    }
    QSettings s;

    m_generalPrefs.data()->setShowTrayIcon(s.value("ShowSystemTray", true).toBool());
    m_generalPrefs.data()->setStartAtLogin(s.value("LaunchAtLogin", true).toBool());

    // Default bounce setting is PM/@-notifications only
    m_notificationPrefs.data()->setBounceSetting((NotificationPreferences::BounceSetting)s.value("BounceDockIcon", NotificationPreferences::BounceOnPM).toInt());

    m_preferencesDialog.data()->setCurrentIndex(0);
    m_preferencesDialog.data()->show();
}

void ZulipWindow::userQuit()
{
    QApplication::quit();
}

void ZulipWindow::trayClicked()
{
    show();
    raise();
    activateWindow();
}

void ZulipWindow::linkClicked(const QUrl& url)
{
    QStringList whitelistedPaths = QStringList() << "/accounts" << "/desktop_home";
    bool whitelisted = false;
    foreach (QString path, whitelistedPaths) {
        if (url.path().startsWith(path)) {
            whitelisted = true;
            break;
        }
    }

    // We need the check for m_start.host() (to make sure we don't display non-zulip.com
    // hosts in the app), and additionally we allow / (the main zulip page) and whitelisted
    // pages (that we need to show in-app, such as Google openid auth)
    if (url.host() == m_start.host() &&
         (url.path() == "/" || whitelisted)) {
        m_ui->webView->load(url);
    } else {
        QDesktopServices::openUrl(url);
    }
}

void ZulipWindow::countUpdated(int newCount)
{
    const int old = m_unreadCount;
    m_unreadCount = newCount;
    if (newCount == old) {
        return;
    }

    m_tray->setIcon(m_renderer->icon(newCount));

    m_platform->unreadCountUpdated(old, newCount);
}

void ZulipWindow::pmCountUpdated(int newCount)
{
    const int old = m_unreadPMCount;
    m_unreadPMCount = newCount;
    if (newCount == old) {
        return;
    }

    if (newCount == 0) {
        stopTrayAnimation();
    } else {
        const QIcon first = m_renderer->icon(m_unreadCount);
        const QIcon second = m_renderer->personIcon();

        startTrayAnimation(QList<QIcon>() << first << second);
    }
}

void ZulipWindow::displayPopup(const QString &title, const QString &content, const QString& source)
{
    // QSystemTrayIcon::showMessage tries to be clever on OS X:
    // If it finds Growl to be installed, it uses AppleScript [!!! >:(]
    // to send a message to GrowlHelperApp, the **old** process name for
    // Growl < 1.3. Starting with Growl >= 1.3, this causes a popup window
    // asking the user to locate Growl, which is terrible.
    // Furthermore, if the user has Growl < 1.3 installed, this will display
    // duplicate desktop notifications, as we properly use the Growl framework.
    // So we use Growl on OS X, and showMessage() elsewhere
    m_platform->desktopNotification(title, content, source);
}

void ZulipWindow::domainSelected(const QString &domain) {
    QString site = domainToUrl(domain);
    if (site.isEmpty())
        return;

    foreach (QAction *action, m_domains.values()) {
        action->setChecked(false);
    }
    m_domains[domain]->setChecked(true);

    QSettings s;
    s.setValue("Domain", domain);
    setUrl(site);
}

void ZulipWindow::reload() {
    webView()->reload();
}

QString ZulipWindow::domainToUrl(const QString& domain) const {
    if (domain.isEmpty()) {
        return QString();
    } else if (domain == "prod") {
        return "https://zulip.com";
    } else if (domain == "staging") {
        return "https://staging.zulip.com";
    } else if (domain == "dev") {
        return "http://localhost:9991";
    } else if (domain.contains("http")) {
        return domain;
    } else {
        qWarning() << "Selected invalid domain?" << domain;
        return QString();
    }
}
