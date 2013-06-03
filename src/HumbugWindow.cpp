#include "HumbugWindow.h"
#include "HumbugTrayIcon.h"
#include "ui_HumbugWindow.h"

#include <iostream>
#include <QMenuBar>
#include <QSystemTrayIcon>
#include <QWebFrame>
#include <QDesktopServices>

#include <stdio.h>

HumbugWindow::HumbugWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::HumbugWindow)
{
    m_ui->setupUi(this);

    start = QUrl("http://localhost:9991/");

    m_ui->webView->load(start);
    this->setMinimumWidth(400);

    statusBar()->hide();

    m_tray = new HumbugTrayIcon(this);
    m_tray->setIcon(QIcon(":/images/hat.svg"));

    QMenu *menu = new QMenu(this);
    QAction *exit_action = menu->addAction("Exit");
    connect(exit_action, SIGNAL(triggered()), this, SLOT(userQuit()));
    connect(m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClicked()));
    m_tray->setContextMenu(menu);
    m_tray->show();

    m_bridge = new HumbugWebBridge(this);
    connect(m_ui->webView->page(), SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(m_ui->webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(addJavaScriptObject()));
    connect(m_bridge, SIGNAL(notificationRequested(QString,QString)), this, SLOT(displayPopup(QString,QString)));
    connect(m_bridge, SIGNAL(countUpdated(int,int)), this, SLOT(updateIcon(int,int)));

    m_ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

}

void HumbugWindow::userQuit()
{
    QApplication::quit();
}

void HumbugWindow::trayClicked()
{
    this->raise();
    this->activateWindow();
}

void HumbugWindow::linkClicked(const QUrl& url)
{
    if (url.host() == start.host()) {
        this->m_ui->webView->load(url);
    } else {
        QDesktopServices::openUrl(url);
    }
    return;
}

void HumbugWindow::addJavaScriptObject()
{
    // Ref: http://www.developer.nokia.com/Community/Wiki/Exposing_QObjects_to_Qt_Webkit

    // Don't expose the JS bridge outside our start domain
    if (m_ui->webView->url().host() != start.host()) {
        return;
    }

    this->m_ui->webView->page()
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
        m_tray->setIcon(QIcon(QString().sprintf(":/images/favicon/favicon-%i.png", current)));
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
