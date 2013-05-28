#include "HumbugWindow.h"
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
}

HumbugWindow::~HumbugWindow()
{
    delete ui;
}
