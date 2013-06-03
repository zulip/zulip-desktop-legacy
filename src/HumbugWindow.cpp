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
    ui(new Ui::HumbugWindow)
{
    ui->setupUi(this);

    start = new QUrl("http://localhost:9991/");

    ui->webView->load(*start);
    this->setMinimumWidth(400);

    statusBar()->hide();

    HumbugTrayIcon *tray = new HumbugTrayIcon(this);
    tray->setIcon(QIcon(":/images/hat.svg"));

    QMenu *menu = new QMenu(this);
    QAction *exit_action = menu->addAction("Exit");
    connect(exit_action, SIGNAL(triggered()), this, SLOT(userQuit()));
    connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClicked()));
    tray->setContextMenu(menu);
    tray->show();

    wb = new HumbugWebBridge(this, ui->webView, tray);
    connect(ui->webView->page(), SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(ui->webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(addJavaScriptObject()));

    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

}

void HumbugWindow::userQuit()
{
    QApplication::quit();
}

void HumbugWindow::trayClicked()
{
    std::cerr << "Raised to top\n";
    std::flush(std::cout);
    this->raise();
    this->activateWindow();
}

void HumbugWindow::linkClicked(QUrl url)
{
    std::cerr << "handling";
    std::flush(std::cout);
    if (url.host() == start->host()) {
        this->ui->webView->load(url);
    } else {
        QDesktopServices::openUrl(url);
    }
    return;
}

void HumbugWindow::addJavaScriptObject()
{
    // Ref: http://www.developer.nokia.com/Community/Wiki/Exposing_QObjects_to_Qt_Webkit
    std::cerr << "maybe adding";

    // Don't expose the JS bridge outside our start domain
    if (this->ui->webView->url().host() != start->host()) {
        std::cerr << "bail";
        std::cerr << this->ui->webView->url().host().toStdString();
        std::cerr << "ed\n";
        std::flush(std::cout);
        return;
    }

    printf("added to %s\n", this->ui->webView->url().toString().toStdString().c_str());
    std::flush(std::cout);

    this->ui->webView->page()
    ->mainFrame()
    ->addToJavaScriptWindowObject("bridge",
                                  this->wb);

    printf("added to %s\n", this->ui->webView->url().toString().toStdString().c_str());
    std::flush(std::cout);
}

bool HumbugWindow::supportsNotifications()
{

}

HumbugWindow::~HumbugWindow()
{
    delete ui;
}
