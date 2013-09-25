#include "ZulipWindow.h"

#include "ZulipApplication.h"
#include "ZulipAboutDialog.h"
#include "IconRenderer.h"
#include "ui_ZulipWindow.h"
#include "Config.h"
#include "PlatformInterface.h"

#include <QDir>
#include <QMenuBar>
#include <QSystemTrayIcon>
#include <QWebFrame>
#include <QCloseEvent>
#include <QWebSettings>
#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <QSystemTrayIcon>
#include <QSignalMapper>
#include <QTimer>
#include <QFontDatabase>
#include <QDebug>

ZulipWindow::ZulipWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::ZulipWindow),
    m_renderer(new IconRenderer(":images/zulip.svg", this)),
    m_trayTimer(new QTimer(this)),
    m_animationStep(0),
    m_domainMapper(new QSignalMapper(this)),
    m_checkForUpdates(0),
    m_startAtLogin(0),
    m_showSysTray(0),
    m_unreadCount(0),
    m_unreadPMCount(0),
    m_platform(new PlatformInterface(this))
{
    m_ui->setupUi(this);

    QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    // Create the directory if it doesn't already exist
    data_dir.mkdir(data_dir.absolutePath());

    QFontDatabase::addApplicationFont(":/zulip.ttc");

    statusBar()->hide();

    setupTray();

    connect(m_ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(m_ui->webView, SIGNAL(desktopNotification(QString,QString)), this, SLOT(displayPopup(QString,QString)));
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

void ZulipWindow::setupTray() {
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(m_renderer->icon());

    // Animation for tray icon is 2seconds per image
    m_trayTimer->setInterval(2000);
    m_trayTimer->setSingleShot(false);
    connect(m_trayTimer, SIGNAL(timeout()), this, SLOT(animateTray()));

    QMenu *menu = new QMenu(this);
    QAction *about_action = menu->addAction("About");
    about_action->setMenuRole(QAction::AboutRole);
    connect(about_action, SIGNAL(triggered()), this, SLOT(showAbout()));

#ifdef Q_OS_WIN
    m_startAtLogin = menu->addAction("Start at Login");
    m_startAtLogin->setCheckable(true);
    connect(m_startAtLogin, SIGNAL(triggered(bool)), this, SLOT(setStartAtLogin(bool)));
#endif

    if (APP->debugMode()) {
        QMenu* domain_menu = new QMenu("Domain", menu);

        QAction* prod = domain_menu->addAction("Production");
        prod->setCheckable(true);
        connect(prod, SIGNAL(triggered()), m_domainMapper, SLOT(map()));
        m_domains["prod"] = prod;

        QAction* staging = domain_menu->addAction("Staging");
        staging->setCheckable(true);
        connect(staging, SIGNAL(triggered()), m_domainMapper, SLOT(map()));
        m_domains["staging"] = staging;

        QAction* dev = domain_menu->addAction("Local");
        dev->setCheckable(true);
        connect(dev, SIGNAL(triggered()), m_domainMapper, SLOT(map()));
        m_domains["dev"] = dev;

        m_domainMapper->setMapping(prod, "prod");
        m_domainMapper->setMapping(staging, "staging");
        m_domainMapper->setMapping(dev, "dev");

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

    QAction* checkForUpdates = about_menu->addAction("Check for Updates...");
    checkForUpdates->setMenuRole(QAction::ApplicationSpecificRole);
    connect(checkForUpdates, SIGNAL(triggered()), m_platform, SLOT(checkForUpdates()));

    m_startAtLogin = about_menu->addAction("Start at Login");
    m_startAtLogin->setCheckable(true);
    m_startAtLogin->setMenuRole(QAction::ApplicationSpecificRole);
    connect(m_startAtLogin, SIGNAL(triggered(bool)), this, SLOT(setStartAtLogin(bool)));
#endif

    m_showSysTray = new QAction("Show Tray Icon", this);
    m_showSysTray->setCheckable(true);
    connect(m_showSysTray, SIGNAL(triggered(bool)), this, SLOT(showSystemTray(bool)));

#if defined(Q_OS_MAC)
    about_menu->addAction(m_showSysTray);
    m_showSysTray->setMenuRole(QAction::ApplicationSpecificRole);
#else
    menu->insertAction(exit_action, m_showSysTray);
#endif
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
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());

    QString domain = settings.value("Domain").toString();
    QString site = domainToUrl(domain);
    if (site.isEmpty()) {
        domain = "prod";
        site = "https://zulip.com";
    }

    m_start = site;
    m_ui->webView->load(m_start);

    if (m_domains.contains(domain))
        m_domains[domain]->setChecked(true);

    bool startAtLoginSetting = settings.value("LaunchAtLogin", true).toBool();
    if (m_startAtLogin) {
        m_startAtLogin->setChecked(startAtLoginSetting);
    }
    m_platform->setStartAtLogin(startAtLoginSetting);

    bool showSysTray = settings.value("ShowSystemTray", true).toBool();
    m_tray->setVisible(showSysTray);
    m_showSysTray->setChecked(showSysTray);
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

void ZulipWindow::userQuit()
{
    QApplication::quit();
}

void ZulipWindow::trayClicked()
{
    raise();
    activateWindow();
}

void ZulipWindow::linkClicked(const QUrl& url)
{
    if (url.host() == m_start.host() && url.path() == "/") {
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

void ZulipWindow::displayPopup(const QString &title, const QString &content)
{
    m_tray->showMessage(title, content);

    m_platform->desktopNotification(title, content);
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

void ZulipWindow::setStartAtLogin(bool start) {
    qDebug () << "Window setting start at login:" << start;
    m_platform->setStartAtLogin(start);

    QSettings s;
    s.setValue("LaunchAtLogin", start);
}

void ZulipWindow::showSystemTray(bool show) {
    m_tray->setVisible(show);

    QSettings s;
    s.setValue("ShowSystemTray", show);
}

QString ZulipWindow::domainToUrl(const QString& domain) const {
    if (domain == "prod") {
        return "https://zulip.com";
    } else if (domain == "staging") {
        return "https://staging.zulip.com";
    } else if (domain == "dev") {
        return "http://localhost:9991";
    } else {
        qWarning() << "Selected invalid domain?" << domain;
        return QString();
    }
}
