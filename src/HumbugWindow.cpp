#include "HumbugWindow.h"
#include "HumbugTrayIcon.h"
#include "ui_HumbugWindow.h"

#include <iostream>
#include <QMenuBar>
#include <QSystemTrayIcon>

HumbugWindow::HumbugWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HumbugWindow)
{
    ui->setupUi(this);

    ui->webView->load(QUrl("http://staging.humbughq.com"));

    statusBar()->hide();

    HumbugTrayIcon *tray = new HumbugTrayIcon(this);
    tray->setIcon(QIcon(":/images/hat.svg"));

    QMenu *menu = new QMenu(this);
    QAction *exit_action = menu->addAction("Exit");
    connect(exit_action, SIGNAL(triggered()), this, SLOT(userQuit()));
    connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClicked()));
    tray->setContextMenu(menu);
    tray->show();

}

void HumbugWindow::userQuit()
{
    QApplication::quit();
}

void HumbugWindow::trayClicked() {
    std::cerr << "Raised to top\n";
    std::flush(std::cout);
    this->raise();
    this->activateWindow();
}

HumbugWindow::~HumbugWindow()
{
    delete ui;
}
