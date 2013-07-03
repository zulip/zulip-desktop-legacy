#include "HumbugWindow.h"

#include "HumbugApplication.h"
#include "HumbugAboutDialog.h"
#include "IconRenderer.h"
#include "ui_HumbugWindow.h"
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

HumbugWindow::HumbugWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::HumbugWindow),
    m_renderer(new IconRenderer(":images/hat.svg", this)),
    m_trayTimer(new QTimer(this)),
    m_animationStep(0),
    m_domainMapper(new QSignalMapper(this)),
    m_unreadCount(0),
    m_unreadPMCount(0),
    m_platform(new PlatformInterface(this))
{
    m_ui->setupUi(this);

    QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    // Create the directory if it doesn't already exist
    data_dir.mkdir(data_dir.absolutePath());

    QFontDatabase::addApplicationFont(":/humbug.ttc");

    statusBar()->hide();

    setupTray();

    connect(m_ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(m_ui->webView, SIGNAL(desktopNotification(QString,QString)), this, SLOT(displayPopup(QString,QString)));
    connect(m_ui->webView, SIGNAL(updateCount(int)), this, SLOT(countUpdated(int)));
    connect(m_ui->webView, SIGNAL(updatePMCount(int)), this, SLOT(pmCountUpdated(int)));    
    connect(m_ui->webView, SIGNAL(bell()), m_platform, SLOT(playSound()));

    readSettings();
}


HumbugWindow::~HumbugWindow()
{
    delete m_ui;
}

HWebView* HumbugWindow::webView() const {
    return m_ui->webView;
}

IconRenderer* HumbugWindow::iconRenderer() const {
    return m_renderer;
}

void HumbugWindow::setupTray() {
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

#ifdef DEBUG_BUILD
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
#endif

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
    QMenu* about_menu = menuBar()->addMenu("Humbug");
    about_menu->addAction(about_action);

    QAction* checkForUpdates = about_menu->addAction("Check for Updates...");
    checkForUpdates->setMenuRole(QAction::ApplicationSpecificRole);
    connect(checkForUpdates, SIGNAL(triggered()), m_platform, SLOT(checkForUpdates()));
#endif
}

void HumbugWindow::startTrayAnimation(const QList<QIcon> &stages) {
    m_animationStages = stages;
    m_animationStep = 0;

    m_tray->setIcon(m_animationStages[m_animationStep]);

    m_trayTimer->start();
}

void HumbugWindow::stopTrayAnimation() {
    m_animationStages.clear();
    m_animationStep = 0;

    m_trayTimer->stop();

    m_tray->setIcon(m_renderer->icon(m_unreadCount));
}

void HumbugWindow::animateTray() {
    m_animationStep = (m_animationStep + 1) % m_animationStages.size();
    m_tray->setIcon(m_animationStages[m_animationStep]);
}

void HumbugWindow::closeEvent(QCloseEvent *ev) {
    QSettings settings;
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());
    QMainWindow::closeEvent(ev);
}

void HumbugWindow::readSettings() {
    QSettings settings;
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());

    QString domain = settings.value("Domain").toString();
    QString site = domainToUrl(domain);
    if (site.isEmpty()) {
        domain = "prod";
        site = "https://humbughq.com";
    }

    m_start = site;
    m_ui->webView->load(m_start);

    if (m_domains.contains(domain))
        m_domains[domain]->setChecked(true);
}

void HumbugWindow::setUrl(const QUrl &url)
{
    m_start = url;

    m_ui->webView->load(url);
}

void HumbugWindow::showAbout()
{
    HumbugAboutDialog *d = new HumbugAboutDialog(this);
    d->show();
}

void HumbugWindow::userQuit()
{
    QApplication::quit();
}

void HumbugWindow::trayClicked()
{
    raise();
    activateWindow();
}

void HumbugWindow::linkClicked(const QUrl& url)
{
    if (url.host() == m_start.host()) {
        m_ui->webView->load(url);
    } else {
        QDesktopServices::openUrl(url);
    }
}

void HumbugWindow::countUpdated(int newCount)
{
    const int old = m_unreadCount;
    m_unreadCount = newCount;
    if (newCount == old) {
        return;
    }

    m_tray->setIcon(m_renderer->icon(newCount));

    m_platform->unreadCountUpdated(old, newCount);
}

void HumbugWindow::pmCountUpdated(int newCount)
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

void HumbugWindow::displayPopup(const QString &title, const QString &content)
{
    m_tray->showMessage(title, content);

    m_platform->desktopNotification(title, content);
}

void HumbugWindow::domainSelected(const QString &domain) {
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


QString HumbugWindow::domainToUrl(const QString& domain) const {
    if (domain == "prod") {
        return "https://humbughq.com";
    } else if (domain == "staging") {
        return "https://staging.humbughq.com";
    } else if (domain == "dev") {
        return "http://localhost:9991";
    } else {
        qWarning() << "Selected invalid domain?" << domain;
        return QString();
    }
}
