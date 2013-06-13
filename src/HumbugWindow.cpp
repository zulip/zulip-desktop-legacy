#include "HumbugWindow.h"
#include "HumbugAboutDialog.h"
#include "HumbugTrayIcon.h"
#include "WheelFilter.h"
#include "ui_HumbugWindow.h"

#include <iostream>
#include <QDir>
#include <QMenuBar>
#include <QSystemTrayIcon>
#include <QWebFrame>
#include <QCloseEvent>
#include <QResource>
#include <QWebSettings>
#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <phonon/MediaObject>
#include <phonon/MediaSource>
#include <phonon/AudioOutput>
#include <stdio.h>

HumbugWindow::HumbugWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::HumbugWindow),
    m_unreadCount(0)
{
    m_ui->setupUi(this);

    m_start = QUrl("https://humbughq.com/");

    QDir data_dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    // Create the directory if it doesn't already exist
    data_dir.mkdir(data_dir.absolutePath());

    m_ui->webView->load(m_start);

    statusBar()->hide();

    setupTray();
    setupSounds();

    connect(m_ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(m_ui->webView, SIGNAL(desktopNotification(QString,QString)), this, SLOT(displayPopup(QString,QString)));
    connect(m_ui->webView, SIGNAL(updateCount(int)), this, SLOT(countUpdated(int)));
    connect(m_ui->webView, SIGNAL(bell()), m_bellsound, SLOT(play()));

    readSettings();
}


HumbugWindow::~HumbugWindow()
{
    delete m_ui;
}

void HumbugWindow::setupTray() {
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
}

void HumbugWindow::setupSounds() {
    m_bellsound = new Phonon::MediaObject(this);
    Phonon::createPath(m_bellsound, new Phonon::AudioOutput(Phonon::MusicCategory, this));

    m_sound_temp.open();
    QResource memory_soundfile(":/humbug.ogg");
    m_sound_temp.write((char*) memory_soundfile.data(), memory_soundfile.size());
    m_sound_temp.flush();
    m_sound_temp.close();

    m_bellsound->setCurrentSource(Phonon::MediaSource(m_sound_temp.fileName()));
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
    } else if (newCount <= 0) {
        m_tray->setIcon(QIcon(":/images/hat.svg"));
    } else if (newCount >= 99) {
        m_tray->setIcon(QIcon(":/images/favicon/favicon-infinite.png"));
    } else {
        m_tray->setIcon(QIcon(QString(":/images/favicon/favicon-%1.png").arg(newCount)));
    }
}

void HumbugWindow::displayPopup(const QString &title, const QString &content)
{
    m_tray->showMessage(title, content);
}
