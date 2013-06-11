#include "HumbugWindow.h"
#include "HumbugAboutDialog.h"
#include "HumbugTrayIcon.h"
#include "ui_HumbugWindow.h"

#include <iostream>
#include <QDir>
#include <QMenuBar>
#include <QSystemTrayIcon>
#include <QWebFrame>
#include <QWebSettings>
#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <phonon/MediaObject>
#include <phonon/MediaSource>
#include <phonon/AudioOutput>
#include <stdio.h>

HumbugWindow::HumbugWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::HumbugWindow)
{
    m_ui->setupUi(this);

    m_start = QUrl("https://humbughq.com/");

    QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    // Create the directory if it doesn't already exist
    data_dir.mkdir(data_dir.absolutePath());

    CookieJar *m_cookies = new CookieJar(data_dir.absoluteFilePath("default.dat"));

    m_ui->webView->load(m_start);
    m_ui->webView->page()->networkAccessManager()->setCookieJar(m_cookies);

    statusBar()->hide();

    m_tray = new HumbugTrayIcon(this);
    m_tray->setIcon(QIcon(":/images/hat.svg"));

    QMenu *menu = new QMenu(this);
    QAction *about_action = menu->addAction("About");
    connect(about_action, SIGNAL(triggered()), this, SLOT(showAbout()));

    QAction *exit_action = menu->addAction("Exit");
    connect(exit_action, SIGNAL(triggered()), this, SLOT(userQuit()));
    connect(m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClicked()));
    m_tray->setContextMenu(menu);
    m_tray->show();


    m_bellsound = new Phonon::MediaObject(this);
    Phonon::createPath(m_bellsound, new Phonon::AudioOutput(Phonon::MusicCategory, this));
    m_bellsound->setCurrentSource(Phonon::MediaSource(QString("/home/lfaraone/orgs/humbug/desktop/src/humbug.ogg")));

    m_bridge = new HumbugWebBridge(this);
    connect(m_ui->webView->page(), SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(m_ui->webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(addJavaScriptObject()));
    connect(m_bridge, SIGNAL(notificationRequested(QString,QString)), this, SLOT(displayPopup(QString,QString)));
    connect(m_bridge, SIGNAL(countUpdated(int,int)), this, SLOT(updateIcon(int,int)));
    connect(m_bridge, SIGNAL(bellTriggered()), m_bellsound, SLOT(play()));

    m_ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

}

void HumbugWindow::setUrl(const QUrl &url)
{
    m_start = url;

    m_ui->webView->load(m_start);
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
    return;
}

void HumbugWindow::addJavaScriptObject()
{
    // Ref: http://www.developer.nokia.com/Community/Wiki/Exposing_QObjects_to_Qt_Webkit

    // Don't expose the JS bridge outside our start domain
    if (m_ui->webView->url().host() != m_start.host()) {
        return;
    }

    m_ui->webView->page()
    ->mainFrame()
    ->addToJavaScriptWindowObject("bridge",
                                  m_bridge);
}

void HumbugWindow::updateIcon(int current, int previous)
{
    if (current == previous) {
        return;
    } else if (current <= 0) {
        m_tray->setIcon(QIcon(":/images/hat.svg"));
    } else if (current >= 99) {
        m_tray->setIcon(QIcon(":/images/favicon/favicon-infinite.png"));
    } else {
        m_tray->setIcon(QIcon(QString(":/images/favicon/favicon-%1.png").arg(current)));
    }
}

void HumbugWindow::displayPopup(const QString &title, const QString &content)
{
    m_tray->showMessage(title, content);
}


HumbugWindow::~HumbugWindow()
{
    delete m_ui;
}
