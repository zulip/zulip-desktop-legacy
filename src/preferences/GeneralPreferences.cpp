#include "GeneralPreferences.h"
#include "ui_GeneralPreferences.h"

#include <QIcon>
GeneralPreferences::GeneralPreferences(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralPreferences)
{
    ui->setupUi(this);

#if !defined(Q_OS_MAC) && !defined (Q_OS_WIN)
    // No start-on-login support on non mac/win
    ui->startAtLogin->hide();
#endif
}

GeneralPreferences::~GeneralPreferences()
{
    delete ui;
}

void GeneralPreferences::setShowTrayIcon(bool showTrayIcon) {
    ui->showTrayIcon->setChecked(showTrayIcon);
}

bool GeneralPreferences::showTrayIcon() const {
    return ui->showTrayIcon->checkState() == Qt::Checked;
}

void GeneralPreferences::setStartAtLogin(bool start) {
    ui->startAtLogin->setChecked(start);
}

bool GeneralPreferences::startAtLogin() const {
    return ui->startAtLogin->checkState() == Qt::Checked;
}
