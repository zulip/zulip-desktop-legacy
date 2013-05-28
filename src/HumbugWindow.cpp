#include "HumbugWindow.h"
#include "HumbugTrayIcon.h"
#include "ui_HumbugWindow.h"

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
    tray->show();

}

HumbugWindow::~HumbugWindow()
{
    delete ui;
}
